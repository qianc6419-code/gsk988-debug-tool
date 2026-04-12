#include "fanucrealtimewidget.h"
#include "fanucframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <cstring>

// ========== Poll Items ==========

const FanucRealtimeWidget::PollItem FanucRealtimeWidget::pollItems[] = {
    {0x01, QByteArray()}, // PI_SYSINFO
    {0x02, QByteArray()}, // PI_RUNINFO
    {0x03, QByteArray()}, // PI_SPINDLE_SPEED
    {0x04, QByteArray()}, // PI_SPINDLE_LOAD
    {0x05, QByteArray()}, // PI_SPINDLE_OVERRIDE
    {0x06, QByteArray()}, // PI_SPINDLE_SPEED_SET
    {0x07, QByteArray()}, // PI_FEED_RATE
    {0x08, QByteArray()}, // PI_FEED_OVERRIDE
    {0x09, QByteArray()}, // PI_FEED_RATE_SET
    {0x0A, QByteArray()}, // PI_PRODUCTS
    {0x0B, QByteArray()}, // PI_RUN_TIME
    {0x0C, QByteArray()}, // PI_CUT_TIME
    {0x0D, QByteArray()}, // PI_CYCLE_TIME
    {0x0E, QByteArray()}, // PI_POWERON_TIME
    {0x0F, QByteArray()}, // PI_PROG_NUM
    {0x10, QByteArray()}, // PI_TOOL_NUM
    {0x11, QByteArray()}, // PI_ALARM
    {0x12, QByteArray()}, // PI_POS_ABS
    {0x13, QByteArray()}, // PI_POS_MACH
    {0x14, QByteArray()}, // PI_POS_REL
    {0x15, QByteArray()}, // PI_POS_RES
};

// ========== Constructor ==========

FanucRealtimeWidget::FanucRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_cycleActive(false)
{
    setupUI();

    m_cycleDelayTimer = new QTimer(this);
    m_cycleDelayTimer->setSingleShot(true);
    connect(m_cycleDelayTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRefreshCheck->isChecked())
            startCycle();
    });
    connect(m_manualRefreshBtn, &QPushButton::clicked, this, [this]() {
        startCycle();
    });
    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_intervalCombo->setEnabled(checked);
        if (!checked)
            m_cycleDelayTimer->stop();
    });
}

// ========== Helpers ==========

static QLabel* makeValueLabel()
{
    auto* lbl = new QLabel("--");
    lbl->setStyleSheet("font-size: 13px; font-weight: bold;");
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return lbl;
}

void FanucRealtimeWidget::setLabelOK(QLabel* label, const QString& text)
{
    label->setText(text);
    label->setStyleSheet("font-size: 13px; font-weight: bold;");
}

void FanucRealtimeWidget::setLabelError(QLabel* label, const QString& /*err*/)
{
    label->setText("--");
    label->setStyleSheet("font-size: 13px; font-weight: bold;");
}

void FanucRealtimeWidget::resetLabel(QLabel* label)
{
    label->setText("--");
    label->setStyleSheet("font-size: 13px; font-weight: bold;");
}

static QString modeStr(qint16 mode)
{
    switch (mode) {
    case 0:  return "MDI";
    case 1:  return "MEM";
    case 3:  return "NO SEARCH";
    case 4:  return "EDIT";
    case 5:  return "HANDLE";
    case 7:  return "INC";
    case 8:  return "JOG";
    case 11: return "REF";
    default: return QString::number(mode);
    }
}

static QString statusStr(qint16 status)
{
    switch (status) {
    case 0:  return "RESET";
    case 1:  return "STOP";
    case 2:  return "HOLD";
    case 3:  return "START";
    case 4:  return "MSTR";
    default: return QString::number(status);
    }
}

static QString formatTime(int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

// ========== UI Setup ==========

void FanucRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // === Control bar ===
    auto* controlBar = new QHBoxLayout;
    m_autoRefreshCheck = new QCheckBox("自动刷新");
    m_autoRefreshCheck->setChecked(true);
    controlBar->addWidget(m_autoRefreshCheck);
    controlBar->addWidget(new QLabel("间隔:"));
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItem("1 秒", 1);
    m_intervalCombo->addItem("2 秒", 2);
    m_intervalCombo->addItem("5 秒", 5);
    m_intervalCombo->addItem("10 秒", 10);
    m_intervalCombo->setCurrentIndex(1); // default 2s
    controlBar->addWidget(m_intervalCombo);
    m_manualRefreshBtn = new QPushButton("手动刷新");
    m_manualRefreshBtn->setFixedWidth(80);
    controlBar->addWidget(m_manualRefreshBtn);
    controlBar->addStretch();
    mainLayout->addLayout(controlBar);

    // === Scroll area ===
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto* scrollWidget = new QWidget;
    auto* grid = new QGridLayout(scrollWidget);

    // Row 0: 系统信息 | 运行状态 | 主轴
    m_sysInfoGroup = new QGroupBox("系统信息");
    {
        auto* l = new QFormLayout;
        m_ncTypeLabel = makeValueLabel();
        m_deviceTypeLabel = makeValueLabel();
        l->addRow("NC型号:", m_ncTypeLabel);
        l->addRow("设备类型:", m_deviceTypeLabel);
        m_sysInfoGroup->setLayout(l);
    }

    m_runStatusGroup = new QGroupBox("运行状态");
    {
        auto* l = new QFormLayout;
        m_modeLabel = makeValueLabel();
        m_runStatusLabel = makeValueLabel();
        m_emgLabel = makeValueLabel();
        m_almLabel = makeValueLabel();
        l->addRow("操作模式:", m_modeLabel);
        l->addRow("运行状态:", m_runStatusLabel);
        l->addRow("急停:", m_emgLabel);
        l->addRow("告警状态:", m_almLabel);
        m_runStatusGroup->setLayout(l);
    }

    m_spindleGroup = new QGroupBox("主轴");
    {
        auto* l = new QFormLayout;
        m_spindleSpeedLabel = makeValueLabel();
        m_spindleLoadLabel = makeValueLabel();
        m_spindleOverrideLabel = makeValueLabel();
        m_spindleSpeedSetLabel = makeValueLabel();
        l->addRow("速度:", m_spindleSpeedLabel);
        l->addRow("负载:", m_spindleLoadLabel);
        l->addRow("倍率:", m_spindleOverrideLabel);
        l->addRow("速度设定值:", m_spindleSpeedSetLabel);
        m_spindleGroup->setLayout(l);
    }

    // Row 1: 进给 | 计数与时间 | 程序与刀具
    m_feedGroup = new QGroupBox("进给");
    {
        auto* l = new QFormLayout;
        m_feedRateLabel = makeValueLabel();
        m_feedOverrideLabel = makeValueLabel();
        m_feedRateSetLabel = makeValueLabel();
        l->addRow("速度:", m_feedRateLabel);
        l->addRow("倍率:", m_feedOverrideLabel);
        l->addRow("速度设定值:", m_feedRateSetLabel);
        m_feedGroup->setLayout(l);
    }

    m_counterTimeGroup = new QGroupBox("计数与时间");
    {
        auto* l = new QFormLayout;
        m_productsLabel = makeValueLabel();
        m_runTimeLabel = makeValueLabel();
        m_cutTimeLabel = makeValueLabel();
        m_cycleTimeLabel = makeValueLabel();
        m_powerOnTimeLabel = makeValueLabel();
        l->addRow("工件数:", m_productsLabel);
        l->addRow("运行时间:", m_runTimeLabel);
        l->addRow("加工时间:", m_cutTimeLabel);
        l->addRow("循环时间:", m_cycleTimeLabel);
        l->addRow("上电时间:", m_powerOnTimeLabel);
        m_counterTimeGroup->setLayout(l);
    }

    m_progToolGroup = new QGroupBox("程序与刀具");
    {
        auto* l = new QFormLayout;
        m_progNumLabel = makeValueLabel();
        m_toolNumLabel = makeValueLabel();
        l->addRow("程序号:", m_progNumLabel);
        l->addRow("刀具号:", m_toolNumLabel);
        m_progToolGroup->setLayout(l);
    }

    // Row 2: 告警 | 坐标
    m_alarmGroup = new QGroupBox("告警");
    {
        auto* l = new QVBoxLayout;
        m_alarmDisplay = new QTextEdit;
        m_alarmDisplay->setReadOnly(true);
        m_alarmDisplay->setMaximumHeight(100);
        m_alarmDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f8f8f8; }");
        m_alarmDisplay->setPlaceholderText("无告警");
        l->addWidget(m_alarmDisplay);
        m_alarmGroup->setLayout(l);
    }

    m_coordGroup = new QGroupBox("坐标");
    {
        auto* l = new QGridLayout;
        // Header row
        l->addWidget(new QLabel(""), 0, 0);
        l->addWidget(new QLabel("X"), 0, 1);
        l->addWidget(new QLabel("Y"), 0, 2);
        l->addWidget(new QLabel("Z"), 0, 3);
        // Row labels
        l->addWidget(new QLabel("绝对坐标:"), 1, 0);
        l->addWidget(new QLabel("机械坐标:"), 2, 0);
        l->addWidget(new QLabel("相对坐标:"), 3, 0);
        l->addWidget(new QLabel("剩余距离:"), 4, 0);
        for (int i = 0; i < 3; ++i) {
            m_posAbs[i] = makeValueLabel();
            m_posMach[i] = makeValueLabel();
            m_posRel[i] = makeValueLabel();
            m_posRes[i] = makeValueLabel();
            l->addWidget(m_posAbs[i], 1, i + 1);
            l->addWidget(m_posMach[i], 2, i + 1);
            l->addWidget(m_posRel[i], 3, i + 1);
            l->addWidget(m_posRes[i], 4, i + 1);
        }
        m_coordGroup->setLayout(l);
    }

    grid->addWidget(m_sysInfoGroup,    0, 0);
    grid->addWidget(m_runStatusGroup,  0, 1);
    grid->addWidget(m_spindleGroup,    0, 2);
    grid->addWidget(m_feedGroup,       1, 0);
    grid->addWidget(m_counterTimeGroup,1, 1);
    grid->addWidget(m_progToolGroup,   1, 2);
    grid->addWidget(m_alarmGroup,      2, 0);
    grid->addWidget(m_coordGroup,      2, 1, 1, 2);

    scrollArea->setWidget(scrollWidget);

    // HEX display
    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(80);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");

    mainLayout->addWidget(scrollArea, 3);
    mainLayout->addWidget(m_hexDisplay, 1);
}

// ========== Cycle Polling ==========

void FanucRealtimeWidget::startCycle()
{
    if (m_cycleActive) return;
    m_cycleActive = true;
    m_pollIndex = 0;
    const auto& item = pollItems[0];
    emit pollRequest(item.cmdCode, item.params);
}

// ========== Update Data ==========

void FanucRealtimeWidget::updateData(const ParsedResponse& resp)
{
    if (!m_cycleActive) return;

    // Determine which poll item this response is for (m_pollIndex is still at the current item)
    int currentIdx = m_pollIndex;

    const QByteArray& data = resp.rawData;

    if (resp.isValid) {
        using FB = FanucFrameBuilder;

        switch (currentIdx) {
        case PI_SYSINFO: { // 0x01 系统信息
            // NC type at offset 8..27 (20 bytes string), device type at offset 28..31
            if (data.size() >= 32) {
                QString ncType = QString::fromLatin1(data.mid(8, 20)).trimmed();
                qint32 devType = FB::decodeInt32(data, 28);
                setLabelOK(m_ncTypeLabel, ncType);
                setLabelOK(m_deviceTypeLabel, QString::number(devType));
            }
            break;
        }
        case PI_RUNINFO: { // 0x02 运行信息
            // mode(2B) at offset 28, status(2B) at 30, emg(2B) at 32, alm(2B) at 34
            if (data.size() >= 36) {
                qint16 mode = FB::decodeInt16(data, 28);
                qint16 status = FB::decodeInt16(data, 30);
                qint16 emg = FB::decodeInt16(data, 32);
                qint16 alm = FB::decodeInt16(data, 34);
                setLabelOK(m_modeLabel, modeStr(mode));
                setLabelOK(m_runStatusLabel, statusStr(status));
                setLabelOK(m_emgLabel, emg != 0 ? "是" : "否");
                setLabelOK(m_almLabel, alm != 0 ? QString("有(%1)").arg(alm) : "无");
            }
            break;
        }
        case PI_SPINDLE_SPEED: { // 0x03 主轴速度
            if (data.size() >= 36) {
                double val = FB::calcValue(data, 28);
                setLabelOK(m_spindleSpeedLabel, QString::number(val, 'f', 0) + " RPM");
            }
            break;
        }
        case PI_SPINDLE_LOAD: { // 0x04 主轴负载
            if (data.size() >= 36) {
                qint32 count = FB::decodeInt32(data, 28);
                if (count >= 1 && data.size() >= 36) {
                    qint32 loadVal = FB::decodeInt32(data, 32);
                    setLabelOK(m_spindleLoadLabel, QString::number(loadVal) + "%");
                }
            }
            break;
        }
        case PI_SPINDLE_OVERRIDE: { // 0x05 主轴倍率
            if (data.size() >= 29) {
                quint8 val = static_cast<quint8>(data[28]);
                setLabelOK(m_spindleOverrideLabel, QString::number(val) + "%");
            }
            break;
        }
        case PI_SPINDLE_SPEED_SET: { // 0x06 主轴速度设定值
            if (data.size() >= 32) {
                qint32 val = FB::decodeInt32(data, 28);
                setLabelOK(m_spindleSpeedSetLabel, QString::number(val) + " RPM");
            }
            break;
        }
        case PI_FEED_RATE: { // 0x07 进给速度
            if (data.size() >= 36) {
                double val = FB::calcValue(data, 28);
                setLabelOK(m_feedRateLabel, QString::number(val, 'f', 0) + " mm/min");
            }
            break;
        }
        case PI_FEED_OVERRIDE: { // 0x08 进给倍率
            if (data.size() >= 29) {
                quint8 val = static_cast<quint8>(data[28]);
                setLabelOK(m_feedOverrideLabel, QString::number(val) + "%");
            }
            break;
        }
        case PI_FEED_RATE_SET: { // 0x09 进给速度设定值
            if (data.size() >= 36) {
                double val = FB::calcValue(data, 28);
                setLabelOK(m_feedRateSetLabel, QString::number(val, 'f', 0) + " mm/min");
            }
            break;
        }
        case PI_PRODUCTS: { // 0x0A 工件数
            if (data.size() >= 36) {
                double val = FB::calcValue(data, 28);
                setLabelOK(m_productsLabel, QString::number(val, 'f', 0));
            }
            break;
        }
        case PI_RUN_TIME: { // 0x0B 运行时间
            if (data.size() >= 32) {
                qint32 val = FB::decodeInt32(data, 28);
                setLabelOK(m_runTimeLabel, formatTime(val));
            }
            break;
        }
        case PI_CUT_TIME: { // 0x0C 加工时间
            if (data.size() >= 32) {
                qint32 val = FB::decodeInt32(data, 28);
                setLabelOK(m_cutTimeLabel, formatTime(val));
            }
            break;
        }
        case PI_CYCLE_TIME: { // 0x0D 循环时间
            if (data.size() >= 32) {
                qint32 val = FB::decodeInt32(data, 28);
                setLabelOK(m_cycleTimeLabel, formatTime(val));
            }
            break;
        }
        case PI_POWERON_TIME: { // 0x0E 上电时间
            if (data.size() >= 32) {
                qint32 val = FB::decodeInt32(data, 28);
                setLabelOK(m_powerOnTimeLabel, formatTime(val));
            }
            break;
        }
        case PI_PROG_NUM: { // 0x0F 程序号
            if (data.size() >= 32) {
                qint32 progNo = FB::decodeInt32(data, 28);
                setLabelOK(m_progNumLabel, "O" + QString::number(progNo));
            }
            break;
        }
        case PI_TOOL_NUM: { // 0x10 刀具号
            if (data.size() >= 36) {
                double val = FB::calcValue(data, 28);
                setLabelOK(m_toolNumLabel, "T" + QString::number(val, 'f', 0));
            }
            break;
        }
        case PI_ALARM: { // 0x11 告警信息
            if (data.size() >= 32) {
                qint32 almCount = FB::decodeInt32(data, 28);
                if (almCount <= 0) {
                    m_alarmDisplay->clear();
                    m_alarmDisplay->setPlaceholderText("无告警");
                } else {
                    QStringList alarms;
                    int offset = 32;
                    for (qint32 i = 0; i < almCount && offset + 8 <= data.size(); ++i) {
                        qint32 almNo = FB::decodeInt32(data, offset);
                        qint32 almType = FB::decodeInt32(data, offset + 4);
                        alarms << QStringLiteral("No.%1 (Type:%2)").arg(almNo).arg(almType);
                        offset += 8;
                    }
                    m_alarmDisplay->setText(alarms.join("\n"));
                }
            }
            break;
        }
        case PI_POS_ABS:   // 0x12 绝对坐标
        case PI_POS_MACH:  // 0x13 机械坐标
        case PI_POS_REL:   // 0x14 相对坐标
        case PI_POS_RES: { // 0x15 剩余距离坐标
            // axis count at offset 28, then each axis is 8 bytes (calcValue)
            QLabel** targetLabels = nullptr;
            switch (currentIdx) {
            case PI_POS_ABS:  targetLabels = m_posAbs;  break;
            case PI_POS_MACH: targetLabels = m_posMach; break;
            case PI_POS_REL:  targetLabels = m_posRel;  break;
            case PI_POS_RES:  targetLabels = m_posRes;  break;
            default: break;
            }
            if (!targetLabels) break;
            if (data.size() >= 32) {
                qint32 axisCount = FB::decodeInt32(data, 28);
                int offset = 32;
                for (qint32 i = 0; i < axisCount && i < 3 && offset + 8 <= data.size(); ++i) {
                    double val = FB::calcValue(data, offset);
                    setLabelOK(targetLabels[i], QString::number(val, 'f', 3));
                    offset += 8;
                }
            }
            break;
        }
        }
    } else {
        // Invalid response - reset the relevant label(s)
        switch (currentIdx) {
        case PI_SYSINFO:
            setLabelError(m_ncTypeLabel, QString());
            setLabelError(m_deviceTypeLabel, QString());
            break;
        case PI_RUNINFO:
            setLabelError(m_modeLabel, QString());
            setLabelError(m_runStatusLabel, QString());
            setLabelError(m_emgLabel, QString());
            setLabelError(m_almLabel, QString());
            break;
        case PI_SPINDLE_SPEED:     setLabelError(m_spindleSpeedLabel, QString()); break;
        case PI_SPINDLE_LOAD:      setLabelError(m_spindleLoadLabel, QString()); break;
        case PI_SPINDLE_OVERRIDE:  setLabelError(m_spindleOverrideLabel, QString()); break;
        case PI_SPINDLE_SPEED_SET: setLabelError(m_spindleSpeedSetLabel, QString()); break;
        case PI_FEED_RATE:         setLabelError(m_feedRateLabel, QString()); break;
        case PI_FEED_OVERRIDE:     setLabelError(m_feedOverrideLabel, QString()); break;
        case PI_FEED_RATE_SET:     setLabelError(m_feedRateSetLabel, QString()); break;
        case PI_PRODUCTS:          setLabelError(m_productsLabel, QString()); break;
        case PI_RUN_TIME:          setLabelError(m_runTimeLabel, QString()); break;
        case PI_CUT_TIME:          setLabelError(m_cutTimeLabel, QString()); break;
        case PI_CYCLE_TIME:        setLabelError(m_cycleTimeLabel, QString()); break;
        case PI_POWERON_TIME:      setLabelError(m_powerOnTimeLabel, QString()); break;
        case PI_PROG_NUM:          setLabelError(m_progNumLabel, QString()); break;
        case PI_TOOL_NUM:          setLabelError(m_toolNumLabel, QString()); break;
        case PI_ALARM:
            m_alarmDisplay->clear();
            m_alarmDisplay->setPlaceholderText("(读取失败)");
            break;
        case PI_POS_ABS:
            for (int i = 0; i < 3; ++i) setLabelError(m_posAbs[i], QString());
            break;
        case PI_POS_MACH:
            for (int i = 0; i < 3; ++i) setLabelError(m_posMach[i], QString());
            break;
        case PI_POS_REL:
            for (int i = 0; i < 3; ++i) setLabelError(m_posRel[i], QString());
            break;
        case PI_POS_RES:
            for (int i = 0; i < 3; ++i) setLabelError(m_posRes[i], QString());
            break;
        }
    }

    // Advance to next poll item
    m_pollIndex++;
    if (m_pollIndex >= PI_COUNT) {
        // Cycle complete
        m_cycleActive = false;
        if (m_autoRefreshCheck->isChecked()) {
            int secs = m_intervalCombo->currentData().toInt();
            m_cycleDelayTimer->start(secs * 1000);
        }
    } else {
        // Send next poll item
        const auto& item = pollItems[m_pollIndex];
        emit pollRequest(item.cmdCode, item.params);
    }
}

// ========== Polling Control ==========

void FanucRealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    startCycle();
}

void FanucRealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_cycleActive = false;
}

// ========== Clear Data ==========

void FanucRealtimeWidget::clearData()
{
    resetLabel(m_ncTypeLabel);
    resetLabel(m_deviceTypeLabel);
    resetLabel(m_modeLabel);
    resetLabel(m_runStatusLabel);
    resetLabel(m_emgLabel);
    resetLabel(m_almLabel);
    resetLabel(m_spindleSpeedLabel);
    resetLabel(m_spindleLoadLabel);
    resetLabel(m_spindleOverrideLabel);
    resetLabel(m_spindleSpeedSetLabel);
    resetLabel(m_feedRateLabel);
    resetLabel(m_feedOverrideLabel);
    resetLabel(m_feedRateSetLabel);
    resetLabel(m_productsLabel);
    resetLabel(m_runTimeLabel);
    resetLabel(m_cutTimeLabel);
    resetLabel(m_cycleTimeLabel);
    resetLabel(m_powerOnTimeLabel);
    resetLabel(m_progNumLabel);
    resetLabel(m_toolNumLabel);
    m_alarmDisplay->clear();
    m_alarmDisplay->setPlaceholderText("无告警");
    for (int i = 0; i < 3; ++i) {
        resetLabel(m_posAbs[i]);
        resetLabel(m_posMach[i]);
        resetLabel(m_posRel[i]);
        resetLabel(m_posRes[i]);
    }
    m_hexDisplay->clear();
}

// ========== HEX Display ==========

void FanucRealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
    m_hexDisplay->append(QString("[%1] %2").arg(isSend ? "TX" : "RX", hex.join(" ")));
}
