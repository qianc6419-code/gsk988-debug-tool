#include "fanucrealtimewidget.h"
#include "fanucframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <cstring>

// ========== Shared Styles ==========

static const char* GROUP_STYLE =
    "QGroupBox {"
    "  font-weight: bold;"
    "  font-size: 12px;"
    "  border: 1px solid #c8c8c8;"
    "  border-radius: 6px;"
    "  margin-top: 12px;"
    "  padding: 10px 6px 6px 6px;"
    "  background: #ffffff;"
    "}"
    "QGroupBox::title {"
    "  subcontrol-origin: margin;"
    "  subcontrol-position: top left;"
    "  padding: 2px 8px;"
    "  color: #333;"
    "}";

static const char* VALUE_STYLE =
    "font-size: 13px; font-weight: bold; font-family: Consolas, 'Courier New', monospace;"
    "color: #1a1a1a; padding: 2px 6px;"
    "background: #f5f6f8; border: 1px solid #dcdfe3; border-radius: 3px;";

static const char* COORD_HEADER_STYLE =
    "font-weight: bold; font-size: 12px; color: #666; padding: 2px 4px;";

static const char* COORD_ROW_STYLE =
    "font-size: 12px; color: #555; padding: 2px 4px;";

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

    // Inter-poll delay: Fanuc devices need ~300ms between commands
    m_interPollTimer = new QTimer(this);
    m_interPollTimer->setSingleShot(true);
    m_interPollTimer->setInterval(300);
    connect(m_interPollTimer, &QTimer::timeout, this, [this]() {
        if (!m_cycleActive) return;
        if (m_pollIndex >= PI_COUNT) {
            m_cycleActive = false;
            if (m_autoRefreshCheck->isChecked()) {
                int secs = m_intervalCombo->currentData().toInt();
                m_cycleDelayTimer->start(secs * 1000);
            }
        } else {
            const auto& item = pollItems[m_pollIndex];
            emit pollRequest(item.cmdCode, item.params);
        }
    });

    connect(m_manualRefreshBtn, &QPushButton::clicked, this, [this]() {
        startCycle();
    });
    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_intervalCombo->setEnabled(checked);
        if (!checked) {
            m_cycleDelayTimer->stop();
            m_interPollTimer->stop();
        }
    });
}

// ========== Helpers ==========

static QLabel* makeValueLabel()
{
    auto* lbl = new QLabel("--");
    lbl->setStyleSheet(VALUE_STYLE);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl->setFixedHeight(22);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return lbl;
}

static QLabel* makeRowLabel(const QString& text)
{
    auto* lbl = new QLabel(text);
    lbl->setStyleSheet(COORD_ROW_STYLE);
    return lbl;
}

static QLabel* makeHeaderLabel(const QString& text)
{
    auto* lbl = new QLabel(text);
    lbl->setStyleSheet(COORD_HEADER_STYLE);
    lbl->setAlignment(Qt::AlignCenter);
    return lbl;
}

void FanucRealtimeWidget::setLabelOK(QLabel* label, const QString& text)
{
    label->setText(text);
    label->setStyleSheet(VALUE_STYLE);
}

void FanucRealtimeWidget::setLabelError(QLabel* label, const QString& /*err*/)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

void FanucRealtimeWidget::resetLabel(QLabel* label)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
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
    mainLayout->setSpacing(6);

    // === Control bar ===
    auto* controlBar = new QHBoxLayout;
    controlBar->setSpacing(8);
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
    scrollArea->setStyleSheet("QScrollArea { border: none; background: #f0f0f0; }");
    auto* scrollWidget = new QWidget;
    scrollWidget->setStyleSheet("background: #f0f0f0;");
    auto* grid = new QGridLayout(scrollWidget);
    grid->setSpacing(8);
    grid->setContentsMargins(6, 6, 6, 6);

    // ===== Row 0: 运行状态 | 主轴 | 进给 =====

    m_runStatusGroup = new QGroupBox("运行状态");
    m_runStatusGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
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
    m_spindleGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_spindleSpeedLabel = makeValueLabel();
        m_spindleLoadLabel = makeValueLabel();
        m_spindleOverrideLabel = makeValueLabel();
        m_spindleSpeedSetLabel = makeValueLabel();
        l->addRow("速度:", m_spindleSpeedLabel);
        l->addRow("负载:", m_spindleLoadLabel);
        l->addRow("倍率:", m_spindleOverrideLabel);
        l->addRow("速度设定:", m_spindleSpeedSetLabel);
        m_spindleGroup->setLayout(l);
    }

    m_feedGroup = new QGroupBox("进给");
    m_feedGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_feedRateLabel = makeValueLabel();
        m_feedOverrideLabel = makeValueLabel();
        m_feedRateSetLabel = makeValueLabel();
        l->addRow("速度:", m_feedRateLabel);
        l->addRow("倍率:", m_feedOverrideLabel);
        l->addRow("速度设定:", m_feedRateSetLabel);
        m_feedGroup->setLayout(l);
    }

    // ===== Row 1: 系统信息 | 计数与时间 | 程序与刀具 =====

    m_sysInfoGroup = new QGroupBox("系统信息");
    m_sysInfoGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_ncTypeLabel = makeValueLabel();
        m_deviceTypeLabel = makeValueLabel();
        l->addRow("NC型号:", m_ncTypeLabel);
        l->addRow("设备类型:", m_deviceTypeLabel);
        m_sysInfoGroup->setLayout(l);
    }

    m_counterTimeGroup = new QGroupBox("计数与时间");
    m_counterTimeGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
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
    m_progToolGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_progNumLabel = makeValueLabel();
        m_toolNumLabel = makeValueLabel();
        l->addRow("程序号:", m_progNumLabel);
        l->addRow("刀具号:", m_toolNumLabel);
        m_progToolGroup->setLayout(l);
    }

    // ===== Row 2: 告警 | 坐标(2col span) =====

    m_alarmGroup = new QGroupBox("告警");
    m_alarmGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QVBoxLayout;
        m_alarmDisplay = new QTextEdit;
        m_alarmDisplay->setReadOnly(true);
        m_alarmDisplay->setMaximumHeight(120);
        m_alarmDisplay->setStyleSheet(
            "QTextEdit { font-size: 12px; background: #fff; "
            "border: 1px solid #dcdfe3; border-radius: 3px; padding: 4px; }");
        m_alarmDisplay->setPlaceholderText("无告警");
        l->addWidget(m_alarmDisplay);
        m_alarmGroup->setLayout(l);
    }

    m_coordGroup = new QGroupBox("坐标");
    m_coordGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QGridLayout;
        l->setSpacing(4);
        l->setContentsMargins(8, 16, 8, 8);
        // Header row
        l->addWidget(makeHeaderLabel(""), 0, 0);
        l->addWidget(makeHeaderLabel("X"), 0, 1);
        l->addWidget(makeHeaderLabel("Y"), 0, 2);
        l->addWidget(makeHeaderLabel("Z"), 0, 3);
        // Separator line
        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #d0d0d0;");
        l->addWidget(sep, 1, 0, 1, 4);
        // Data rows
        l->addWidget(makeRowLabel("绝对坐标:"), 2, 0);
        l->addWidget(makeRowLabel("机械坐标:"), 3, 0);
        l->addWidget(makeRowLabel("相对坐标:"), 4, 0);
        l->addWidget(makeRowLabel("剩余距离:"), 5, 0);
        for (int i = 0; i < 3; ++i) {
            m_posAbs[i] = makeValueLabel();
            m_posMach[i] = makeValueLabel();
            m_posRel[i] = makeValueLabel();
            m_posRes[i] = makeValueLabel();
            l->addWidget(m_posAbs[i], 2, i + 1);
            l->addWidget(m_posMach[i], 3, i + 1);
            l->addWidget(m_posRel[i], 4, i + 1);
            l->addWidget(m_posRes[i], 5, i + 1);
        }
        m_coordGroup->setLayout(l);
    }

    // Grid layout
    grid->addWidget(m_runStatusGroup,  0, 0);
    grid->addWidget(m_spindleGroup,    0, 1);
    grid->addWidget(m_feedGroup,       0, 2);
    grid->addWidget(m_sysInfoGroup,    1, 0);
    grid->addWidget(m_counterTimeGroup,1, 1);
    grid->addWidget(m_progToolGroup,   1, 2);
    grid->addWidget(m_alarmGroup,      2, 0);
    grid->addWidget(m_coordGroup,      2, 1, 1, 2);

    // Make columns equal width
    for (int c = 0; c < 3; ++c)
        grid->setColumnStretch(c, 1);

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
        // NOTE: rawData = frame.mid(10), so offsets = frame_offset - 10
        // NC direct commands: data at rawData[18], param reads: data at rawData[26]

        switch (currentIdx) {
        case PI_SYSINFO: { // 0x01 系统信息 — ncType at rawData[22] (2 chars), deviceType at rawData[24] (2 chars)
            if (data.size() >= 26) {
                char ncType[3] = {0};
                char deviceType[3] = {0};
                memcpy(ncType, data.constData() + 22, 2);
                memcpy(deviceType, data.constData() + 24, 2);
                setLabelOK(m_ncTypeLabel, QString(ncType));
                setLabelOK(m_deviceTypeLabel, QString(deviceType));
            }
            break;
        }
        case PI_RUNINFO: { // 0x02 运行信息 — mode at rawData[18], status at [20], emg at [26], alm at [28]
            if (data.size() >= 30) {
                qint16 mode = FB::decodeInt16(data, 18);
                qint16 status = FB::decodeInt16(data, 20);
                qint16 emg = FB::decodeInt16(data, 26);
                qint16 alm = FB::decodeInt16(data, 28);
                setLabelOK(m_modeLabel, modeStr(mode));
                setLabelOK(m_runStatusLabel, statusStr(status));
                setLabelOK(m_emgLabel, emg != 0 ? "是" : "否");
                setLabelOK(m_almLabel, alm != 0 ? QString("有(%1)").arg(alm) : "无");
            }
            break;
        }
        case PI_SPINDLE_SPEED: { // 0x03 主轴速度 — calcValue at rawData[18]
            if (data.size() >= 26) {
                double val = FB::calcValue(data, 18);
                setLabelOK(m_spindleSpeedLabel, QString::number(val, 'f', 0) + " RPM");
            }
            break;
        }
        case PI_SPINDLE_LOAD: { // 0x04 主轴负载 — calcValue at rawData[18]
            if (data.size() >= 26) {
                double val = FB::calcValue(data, 18);
                setLabelOK(m_spindleLoadLabel, QString::number(val, 'f', 2) + "%");
            }
            break;
        }
        case PI_SPINDLE_OVERRIDE: { // 0x05 主轴倍率 — byte at rawData[18]
            if (data.size() >= 19) {
                quint8 val = static_cast<quint8>(data[18]);
                setLabelOK(m_spindleOverrideLabel, QString::number(val) + "%");
            }
            break;
        }
        case PI_SPINDLE_SPEED_SET: { // 0x06 主轴速度设定值 — int32 at rawData[18]
            if (data.size() >= 22) {
                qint32 val = FB::decodeInt32(data, 18);
                setLabelOK(m_spindleSpeedSetLabel, QString::number(val) + " RPM");
            }
            break;
        }
        case PI_FEED_RATE: { // 0x07 进给速度 — calcValue at rawData[18]
            if (data.size() >= 26) {
                double val = FB::calcValue(data, 18);
                setLabelOK(m_feedRateLabel, QString::number(val, 'f', 0) + " mm/min");
            }
            break;
        }
        case PI_FEED_OVERRIDE: { // 0x08 进给倍率 — 255 - byte at rawData[18]
            if (data.size() >= 19) {
                int val = 255 - static_cast<quint8>(data[18]);
                setLabelOK(m_feedOverrideLabel, QString::number(val) + "%");
            }
            break;
        }
        case PI_FEED_RATE_SET: { // 0x09 进给速度设定值 — calcValue at rawData[18]
            if (data.size() >= 26) {
                double val = FB::calcValue(data, 18);
                setLabelOK(m_feedRateSetLabel, QString::number(val, 'f', 0) + " mm/min");
            }
            break;
        }
        case PI_PRODUCTS: { // 0x0A 工件数 — calcValue at rawData[18]
            if (data.size() >= 26) {
                double val = FB::calcValue(data, 18);
                setLabelOK(m_productsLabel, QString::number(val, 'f', 0));
            }
            break;
        }
        case PI_RUN_TIME: { // 0x0B 运行时间 — multi-block: ms=calcValue(rawData[26]), min=calcValue(rawData[26+block1Len])
            if (data.size() >= 34) {
                double ms = FB::calcValue(data, 26);
                qint16 block1Len = FB::decodeInt16(data, 2);
                double minutes = 0;
                if (data.size() >= 26 + block1Len + 8)
                    minutes = FB::calcValue(data, 26 + block1Len);
                int seconds = static_cast<int>(minutes) * 60 + static_cast<int>(ms / 1000);
                setLabelOK(m_runTimeLabel, formatTime(seconds));
            }
            break;
        }
        case PI_CUT_TIME: { // 0x0C 加工时间
            if (data.size() >= 34) {
                double ms = FB::calcValue(data, 26);
                qint16 block1Len = FB::decodeInt16(data, 2);
                double minutes = 0;
                if (data.size() >= 26 + block1Len + 8)
                    minutes = FB::calcValue(data, 26 + block1Len);
                int seconds = static_cast<int>(minutes) * 60 + static_cast<int>(ms / 1000);
                setLabelOK(m_cutTimeLabel, formatTime(seconds));
            }
            break;
        }
        case PI_CYCLE_TIME: { // 0x0D 循环时间
            if (data.size() >= 34) {
                double ms = FB::calcValue(data, 26);
                qint16 block1Len = FB::decodeInt16(data, 2);
                double minutes = 0;
                if (data.size() >= 26 + block1Len + 8)
                    minutes = FB::calcValue(data, 26 + block1Len);
                int seconds = static_cast<int>(minutes) * 60 + static_cast<int>(ms / 1000);
                setLabelOK(m_cycleTimeLabel, formatTime(seconds));
            }
            break;
        }
        case PI_POWERON_TIME: { // 0x0E 上电时间 — single param: calcValue at rawData[26], min*60
            if (data.size() >= 34) {
                double minutes = FB::calcValue(data, 26);
                int seconds = static_cast<int>(minutes) * 60;
                setLabelOK(m_powerOnTimeLabel, formatTime(seconds));
            }
            break;
        }
        case PI_PROG_NUM: { // 0x0F 程序号 — int32 at rawData[18]
            if (data.size() >= 22) {
                qint32 progNo = FB::decodeInt32(data, 18);
                setLabelOK(m_progNumLabel, "O" + QString::number(progNo));
            }
            break;
        }
        case PI_TOOL_NUM: { // 0x10 刀具号 — calcValue at rawData[18]
            if (data.size() >= 26) {
                double val = FB::calcValue(data, 18);
                setLabelOK(m_toolNumLabel, "T" + QString::number(val, 'f', 0));
            }
            break;
        }
        case PI_ALARM: { // 0x11 告警 — dataLength at rawData[14], num=dataLen/48, each 48B
            if (data.size() >= 22) {
                qint32 dataLength = FB::decodeInt32(data, 14);
                unsigned int num = static_cast<unsigned int>(dataLength) / 48;
                if (num == 0) {
                    m_alarmDisplay->clear();
                    m_alarmDisplay->setPlaceholderText("无告警");
                } else {
                    QStringList alarms;
                    for (unsigned int i = 0; i < num; ++i) {
                        int base = 18 + static_cast<int>(i) * 48;
                        if (base + 48 > data.size()) break;
                        qint32 almNo = FB::decodeInt32(data, base);
                        qint32 almType = FB::decodeInt32(data, base + 4);
                        qint32 almLen = FB::decodeInt32(data, base + 12);
                        if (almLen > 0 && almLen <= 32) {
                            QByteArray msg = data.mid(base + 16, almLen);
                            alarms << QStringLiteral("No.%1 (Type:%2) %3")
                                      .arg(almNo).arg(almType).arg(QString::fromLatin1(msg));
                        } else {
                            alarms << QStringLiteral("No.%1 (Type:%2)").arg(almNo).arg(almType);
                        }
                    }
                    m_alarmDisplay->setText(alarms.join("\n"));
                }
            }
            break;
        }
        case PI_POS_ABS:   // 0x12 绝对坐标 — x=calcValue(rawData[18]), y=calcValue(rawData[26]), z=calcValue(rawData[34])
        case PI_POS_MACH:  // 0x13 机械坐标
        case PI_POS_REL:   // 0x14 相对坐标
        case PI_POS_RES: { // 0x15 剩余距离坐标
            QLabel** targetLabels = nullptr;
            switch (currentIdx) {
            case PI_POS_ABS:  targetLabels = m_posAbs;  break;
            case PI_POS_MACH: targetLabels = m_posMach; break;
            case PI_POS_REL:  targetLabels = m_posRel;  break;
            case PI_POS_RES:  targetLabels = m_posRes;  break;
            default: break;
            }
            if (!targetLabels) break;
            if (data.size() >= 26) {
                double x = FB::calcValue(data, 18);
                setLabelOK(targetLabels[0], QString::number(x, 'f', 3));
                if (data.size() >= 34) {
                    double y = FB::calcValue(data, 26);
                    setLabelOK(targetLabels[1], QString::number(y, 'f', 3));
                }
                if (data.size() >= 42) {
                    double z = FB::calcValue(data, 34);
                    setLabelOK(targetLabels[2], QString::number(z, 'f', 3));
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

    // Advance to next poll item (with inter-poll delay for device stability)
    m_pollIndex++;
    if (m_pollIndex >= PI_COUNT) {
        // Cycle complete
        m_cycleActive = false;
        if (m_autoRefreshCheck->isChecked()) {
            int secs = m_intervalCombo->currentData().toInt();
            m_cycleDelayTimer->start(secs * 1000);
        }
    } else {
        // Delay before sending next request to prevent device disconnection
        m_interPollTimer->start();
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
    m_interPollTimer->stop();
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
