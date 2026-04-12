#include "gsk988realtimewidget.h"
#include "gsk988protocol.h"
#include <QGridLayout>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <cstring>

// ========== Poll Items ==========
const Gsk988RealtimeWidget::PollItem Gsk988RealtimeWidget::pollItems[] = {
    // Device status
    {0x10, QByteArray(1, '\0')},            // 0:  运行模式
    {0x11, QByteArray(1, '\0')},            // 1:  运行状态
    {0x22, QByteArray(1, '\0')},            // 2:  暂停状态
    // Feed speeds/override
    {0x1A, QByteArray("\x00\x00\x00", 3)},  // 3:  进给编程速度
    {0x1A, QByteArray("\x01\x00\x00", 3)},  // 4:  进给实际速度
    {0x1A, QByteArray("\x02\x00\x00", 3)},  // 5:  进给倍率
    // Spindle speeds/override
    {0x1A, QByteArray("\x03\x00\x00", 3)},  // 6:  主轴编程速度
    {0x1A, QByteArray("\x04\x00\x00", 3)},  // 7:  主轴实际速度
    {0x1A, QByteArray("\x05\x00\x00", 3)},  // 8:  主轴倍率
    // Other overrides
    {0x1A, QByteArray("\x06\x00\x00", 3)},  // 9:  快速倍率
    {0x1A, QByteArray("\x07\x00\x00", 3)},  // 10: 手动倍率
    {0x1A, QByteArray("\x08\x00\x00", 3)},  // 11: 手轮倍率
    // Coordinates (0x15 with axis type)
    {0x15, QByteArray("\x00\x00", 2)},      // 12: 机械坐标
    {0x15, QByteArray("\x00\x01", 2)},      // 13: 绝对坐标
    {0x15, QByteArray("\x00\x02", 2)},      // 14: 相对坐标
    {0x15, QByteArray("\x00\x03", 2)},      // 15: 余移动量
    // Machining info
    {0x16, QByteArray(1, '\0')},            // 16: 加工件数
    {0x17, QByteArray(1, '\0')},            // 17: 当前刀号
    {0x18, QByteArray(1, '\0')},            // 18: 运行时间
    {0x19, QByteArray(1, '\0')},            // 19: 切削时间
    // Program info
    {0x12, QByteArray(1, '\0')},            // 20: 运行程序名
    {0x1D, QByteArray(1, '\0')},            // 21: 当前段号
    {0x23, QByteArray(1, '\0')},            // 22: CNC执行行号
    // Modal info
    {0x1B, QByteArray(1, '\0')},            // 23: G模态
    {0x1C, QByteArray(1, '\0')},            // 24: M模态
    // Alarm
    {0x81, QByteArray()},                   // 25: CNC报警数
};

// ========== Constructor ==========

Gsk988RealtimeWidget::Gsk988RealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_pollCount(PI_COUNT)
    , m_cycleActive(false)
{
    setupUI();

    // Timer for auto-refresh interval (between cycles)
    m_cycleDelayTimer = new QTimer(this);
    m_cycleDelayTimer->setSingleShot(true);
    connect(m_cycleDelayTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRefreshCheck->isChecked())
            startCycle();
    });

    // Manual refresh
    connect(m_manualRefreshBtn, &QPushButton::clicked, this, [this]() {
        startCycle();
    });

    // Auto-refresh toggle
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
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return lbl;
}

static quint32 readLE32(const QByteArray& ba, int offset)
{
    return static_cast<quint8>(ba[offset]) |
           (static_cast<quint8>(ba[offset + 1]) << 8) |
           (static_cast<quint8>(ba[offset + 2]) << 16) |
           (static_cast<quint8>(ba[offset + 3]) << 24);
}

static quint16 readLE16(const QByteArray& ba, int offset)
{
    return static_cast<quint8>(ba[offset]) |
           (static_cast<quint8>(ba[offset + 1]) << 8);
}

static float readFloatVal(const QByteArray& ba, int offset)
{
    float f;
    std::memcpy(&f, ba.constData() + offset, sizeof(f));
    return f;
}

void Gsk988RealtimeWidget::setLabelOK(QLabel* label, const QString& text)
{
    label->setText(text);
    label->setStyleSheet("");
}

void Gsk988RealtimeWidget::setLabelError(QLabel* label, const QString& /*err*/)
{
    label->setText("--");
    label->setStyleSheet("");
}

void Gsk988RealtimeWidget::resetLabel(QLabel* label)
{
    label->setText("--");
    label->setStyleSheet("");
}

// ========== UI Setup ==========

static QGroupBox* makeCoordCard(const QString& title, QLabel* labels[3])
{
    auto* card = new QGroupBox(title);
    auto* layout = new QFormLayout;
    const char* axisNames[] = {"X:", "Y:", "Z:"};
    for (int i = 0; i < 3; ++i) {
        labels[i] = makeValueLabel();
        layout->addRow(axisNames[i], labels[i]);
    }
    card->setLayout(layout);
    return card;
}

void Gsk988RealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // === Control bar ===
    auto* controlBar = new QHBoxLayout;
    m_autoRefreshCheck = new QCheckBox("自动刷新");
    m_autoRefreshCheck->setChecked(true);

    controlBar->addWidget(m_autoRefreshCheck);

    controlBar->addWidget(new QLabel("间隔:"));
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItem("2 秒", 2);
    m_intervalCombo->addItem("5 秒", 5);
    m_intervalCombo->addItem("10 秒", 10);
    m_intervalCombo->addItem("30 秒", 30);
    m_intervalCombo->addItem("60 秒", 60);
    m_intervalCombo->setCurrentIndex(1); // default 5s
    controlBar->addWidget(m_intervalCombo);

    m_manualRefreshBtn = new QPushButton("手动刷新");
    m_manualRefreshBtn->setFixedWidth(80);
    controlBar->addWidget(m_manualRefreshBtn);
    controlBar->addStretch();

    mainLayout->addLayout(controlBar);

    // === Scroll area for cards ===
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto* scrollWidget = new QWidget;
    auto* grid = new QGridLayout(scrollWidget);

    // Row 0: 设备状态 | 进给 | 主轴
    m_deviceCard = new QGroupBox("设备状态");
    {
        auto* l = new QFormLayout;
        m_runModeLabel = makeValueLabel();
        m_runStatusLabel = makeValueLabel();
        m_pauseLabel = makeValueLabel();
        l->addRow("运行模式:", m_runModeLabel);
        l->addRow("运行状态:", m_runStatusLabel);
        l->addRow("暂停状态:", m_pauseLabel);
        m_deviceCard->setLayout(l);
    }

    m_feedCard = new QGroupBox("进给");
    {
        auto* l = new QFormLayout;
        m_feedProgLabel = makeValueLabel();
        m_feedActualLabel = makeValueLabel();
        m_feedOverrideLabel = makeValueLabel();
        l->addRow("编程速度:", m_feedProgLabel);
        l->addRow("实际速度:", m_feedActualLabel);
        l->addRow("进给倍率:", m_feedOverrideLabel);
        m_feedCard->setLayout(l);
    }

    m_spindleCard = new QGroupBox("主轴");
    {
        auto* l = new QFormLayout;
        m_spindleProgLabel = makeValueLabel();
        m_spindleActualLabel = makeValueLabel();
        m_spindleOverrideLabel = makeValueLabel();
        l->addRow("编程速度:", m_spindleProgLabel);
        l->addRow("实际速度:", m_spindleActualLabel);
        l->addRow("主轴倍率:", m_spindleOverrideLabel);
        m_spindleCard->setLayout(l);
    }

    // Row 1: 其他倍率 | 加工信息 | 程序信息
    m_overrideCard = new QGroupBox("其他倍率");
    {
        auto* l = new QFormLayout;
        m_rapidOverrideLabel = makeValueLabel();
        m_manualOverrideLabel = makeValueLabel();
        m_handwheelOverrideLabel = makeValueLabel();
        l->addRow("快速倍率:", m_rapidOverrideLabel);
        l->addRow("手动倍率:", m_manualOverrideLabel);
        l->addRow("手轮倍率:", m_handwheelOverrideLabel);
        m_overrideCard->setLayout(l);
    }

    m_machiningCard = new QGroupBox("加工信息");
    {
        auto* l = new QFormLayout;
        m_pieceCountLabel = makeValueLabel();
        m_runTimeLabel = makeValueLabel();
        m_cutTimeLabel = makeValueLabel();
        m_toolNoLabel = makeValueLabel();
        l->addRow("加工件数:", m_pieceCountLabel);
        l->addRow("运行时间:", m_runTimeLabel);
        l->addRow("切削时间:", m_cutTimeLabel);
        l->addRow("当前刀号:", m_toolNoLabel);
        m_machiningCard->setLayout(l);
    }

    m_programCard = new QGroupBox("程序信息");
    {
        auto* l = new QFormLayout;
        m_programNameLabel = makeValueLabel();
        m_segmentLabel = makeValueLabel();
        m_execLineLabel = makeValueLabel();
        l->addRow("程序名:", m_programNameLabel);
        l->addRow("当前段号:", m_segmentLabel);
        l->addRow("执行行号:", m_execLineLabel);
        m_programCard->setLayout(l);
    }

    // Row 2: 机械坐标 | 绝对坐标 | 相对坐标
    m_machineCoordCard = makeCoordCard("机械坐标", m_machineCoord);
    m_absCoordCard     = makeCoordCard("绝对坐标", m_absCoord);
    m_relCoordCard     = makeCoordCard("相对坐标", m_relCoord);

    // Row 3: 余移动量 | 模态信息 | 当前报警
    m_remainCoordCard = makeCoordCard("余移动量", m_remainCoord);

    m_modalCard = new QGroupBox("模态信息");
    {
        auto* l = new QFormLayout;
        m_gModalLabel = makeValueLabel();
        m_mModalLabel = makeValueLabel();
        l->addRow("G模态:", m_gModalLabel);
        l->addRow("M模态:", m_mModalLabel);
        m_modalCard->setLayout(l);
    }

    m_alarmCard = new QGroupBox("当前报警");
    {
        auto* l = new QFormLayout;
        m_errorCountLabel = makeValueLabel();
        m_warnCountLabel = makeValueLabel();
        l->addRow("错误数:", m_errorCountLabel);
        l->addRow("警告数:", m_warnCountLabel);
        m_alarmCard->setLayout(l);
    }

    grid->addWidget(m_deviceCard,      0, 0);
    grid->addWidget(m_feedCard,        0, 1);
    grid->addWidget(m_spindleCard,     0, 2);
    grid->addWidget(m_overrideCard,    1, 0);
    grid->addWidget(m_machiningCard,   1, 1);
    grid->addWidget(m_programCard,     1, 2);
    grid->addWidget(m_machineCoordCard,2, 0);
    grid->addWidget(m_absCoordCard,    2, 1);
    grid->addWidget(m_relCoordCard,    2, 2);
    grid->addWidget(m_remainCoordCard, 3, 0);
    grid->addWidget(m_modalCard,       3, 1);
    grid->addWidget(m_alarmCard,       3, 2);

    scrollArea->setWidget(scrollWidget);

    // HEX display
    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(100);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");

    mainLayout->addWidget(scrollArea, 3);
    mainLayout->addWidget(m_hexDisplay, 1);
}

// ========== Cycle Polling ==========

void Gsk988RealtimeWidget::startCycle()
{
    if (m_cycleActive) return; // already cycling
    m_cycleActive = true;
    m_pollIndex = 0;
    const auto& item = pollItems[0];
    emit pollRequest(item.cmdCode, item.params);
}

// ========== Update Routing ==========

static QString runModeStr(quint32 v)
{
    switch (v) {
    case 0: return "手动";
    case 1: return "自动";
    case 2: return "MDI";
    default: return QString::number(v);
    }
}

static QString runStatusStr(quint32 v)
{
    switch (v) {
    case 0: return "停止";
    case 1: return "运行中";
    case 2: return "暂停";
    default: return QString::number(v);
    }
}

void Gsk988RealtimeWidget::updateData(const ParsedResponse& resp)
{
    // Advance cycle if active
    if (m_cycleActive) {
        m_pollIndex++;
        if (m_pollIndex >= m_pollCount) {
            // Cycle complete
            m_cycleActive = false;
            // Schedule next cycle if auto-refresh
            if (m_autoRefreshCheck->isChecked()) {
                int secs = m_intervalCombo->currentData().toInt();
                m_cycleDelayTimer->start(secs * 1000);
            }
        } else {
            // Send next item in cycle
            const auto& item = pollItems[m_pollIndex];
            emit pollRequest(item.cmdCode, item.params);
        }
    }

    switch (resp.cmdCode) {
    case 0x10: case 0x11: case 0x22:
        updateDeviceCard(resp); break;
    case 0x1A:
        updateSpeedCard(resp); break;
    case 0x15:
        updateCoordCard(resp); break;
    case 0x16: case 0x17: case 0x18: case 0x19:
        updateMachiningCard(resp); break;
    case 0x12: case 0x1D: case 0x23:
    case 0x1B: case 0x1C:
        updateProgramCard(resp); break;
    case 0x81:
        updateAlarmCard(resp); break;
    default: break;
    }
}

// ========== Device Card ==========

void Gsk988RealtimeWidget::updateDeviceCard(const ParsedResponse& resp)
{
    if (!resp.isValid) {
        switch (resp.cmdCode) {
        case 0x10: setLabelError(m_runModeLabel, QString()); break;
        case 0x11: setLabelError(m_runStatusLabel, QString()); break;
        case 0x22: setLabelError(m_pauseLabel, QString()); break;
        }
        return;
    }
    switch (resp.cmdCode) {
    case 0x10:
        if (resp.rawData.size() >= 4)
            setLabelOK(m_runModeLabel, runModeStr(readLE32(resp.rawData, 0)));
        break;
    case 0x11:
        if (resp.rawData.size() >= 4)
            setLabelOK(m_runStatusLabel, runStatusStr(readLE32(resp.rawData, 0)));
        break;
    case 0x22:
        if (resp.rawData.size() >= 4) {
            quint32 v = readLE32(resp.rawData, 0);
            setLabelOK(m_pauseLabel, v == 0 ? "否" : QString("是 (%1)").arg(v));
        }
        break;
    }
}

// ========== Speed Card (0x1A) ==========

void Gsk988RealtimeWidget::updateSpeedCard(const ParsedResponse& resp)
{
    int prevIdx = m_pollIndex > 0 ? m_pollIndex - 1 : m_pollCount - 1;

    QLabel* target = nullptr;
    QString unit;
    bool isPercent = false;

    switch (prevIdx) {
    case PI_FEED_PROG:      target = m_feedProgLabel;      unit = " mm/min"; break;
    case PI_FEED_ACTUAL:    target = m_feedActualLabel;    unit = " mm/min"; break;
    case PI_FEED_OVERRIDE:  target = m_feedOverrideLabel;  isPercent = true; break;
    case PI_SPINDLE_PROG:   target = m_spindleProgLabel;   unit = " RPM"; break;
    case PI_SPINDLE_ACTUAL: target = m_spindleActualLabel; unit = " RPM"; break;
    case PI_SPINDLE_OVERRIDE: target = m_spindleOverrideLabel; isPercent = true; break;
    case PI_RAPID_OVERRIDE:   target = m_rapidOverrideLabel;   isPercent = true; break;
    case PI_MANUAL_OVERRIDE:  target = m_manualOverrideLabel;  isPercent = true; break;
    case PI_HANDWHEEL_OVERRIDE: target = m_handwheelOverrideLabel; isPercent = true; break;
    default: return;
    }

    if (!resp.isValid) {
        setLabelError(target, QString());
        return;
    }

    if (resp.rawData.size() >= 4) {
        float val = readFloatVal(resp.rawData, 0);
        if (isPercent)
            setLabelOK(target, QString::number(val, 'f', 1) + "%");
        else
            setLabelOK(target, QString::number(qRound(val)) + unit);
    }
}

// ========== Coordinate Card (0x15) ==========

void Gsk988RealtimeWidget::updateCoordCard(const ParsedResponse& resp)
{
    int prevIdx = m_pollIndex > 0 ? m_pollIndex - 1 : m_pollCount - 1;

    QLabel** targetLabels = nullptr;
    switch (prevIdx) {
    case PI_COORD_MACHINE: targetLabels = m_machineCoord; break;
    case PI_COORD_ABS:     targetLabels = m_absCoord;     break;
    case PI_COORD_REL:     targetLabels = m_relCoord;     break;
    case PI_COORD_REMAIN:  targetLabels = m_remainCoord;  break;
    default: return;
    }

    if (!resp.isValid) {
        setLabelError(targetLabels[0], QString());
        resetLabel(targetLabels[1]);
        resetLabel(targetLabels[2]);
        return;
    }

    if (resp.rawData.size() < 2) return;

    quint16 count = readLE16(resp.rawData, 0);
    int offset = 2;
    for (int i = 0; i < count && i < 3 && offset + 4 <= resp.rawData.size(); ++i) {
        float val = readFloatVal(resp.rawData, offset);
        setLabelOK(targetLabels[i], QString::number(val, 'f', 3));
        offset += 4;
    }
}

// ========== Machining Card ==========

void Gsk988RealtimeWidget::updateMachiningCard(const ParsedResponse& resp)
{
    if (!resp.isValid) {
        switch (resp.cmdCode) {
        case 0x16: setLabelError(m_pieceCountLabel, QString()); break;
        case 0x17: setLabelError(m_toolNoLabel, QString()); break;
        case 0x18: setLabelError(m_runTimeLabel, QString()); break;
        case 0x19: setLabelError(m_cutTimeLabel, QString()); break;
        }
        return;
    }
    switch (resp.cmdCode) {
    case 0x16:
        if (resp.rawData.size() >= 4)
            setLabelOK(m_pieceCountLabel, QString::number(readLE32(resp.rawData, 0)));
        break;
    case 0x17:
        if (resp.rawData.size() >= 4) {
            quint16 tool = readLE16(resp.rawData, 0);
            quint16 comp = readLE16(resp.rawData, 2);
            setLabelOK(m_toolNoLabel, QString("刀号:%1 刀补:%2").arg(tool).arg(comp));
        }
        break;
    case 0x18:
        if (resp.rawData.size() >= 4) {
            quint32 s = readLE32(resp.rawData, 0);
            setLabelOK(m_runTimeLabel, QString("%1h %2m %3s").arg(s/3600).arg((s%3600)/60).arg(s%60));
        }
        break;
    case 0x19:
        if (resp.rawData.size() >= 4) {
            quint32 s = readLE32(resp.rawData, 0);
            setLabelOK(m_cutTimeLabel, QString("%1h %2m %3s").arg(s/3600).arg((s%3600)/60).arg(s%60));
        }
        break;
    }
}

// ========== Program Card ==========

void Gsk988RealtimeWidget::updateProgramCard(const ParsedResponse& resp)
{
    if (!resp.isValid) {
        switch (resp.cmdCode) {
        case 0x12: setLabelError(m_programNameLabel, QString()); break;
        case 0x1D: setLabelError(m_segmentLabel, QString()); break;
        case 0x23: setLabelError(m_execLineLabel, QString()); break;
        case 0x1B: setLabelError(m_gModalLabel, QString()); break;
        case 0x1C: setLabelError(m_mModalLabel, QString()); break;
        }
        return;
    }
    switch (resp.cmdCode) {
    case 0x12:
        if (resp.rawData.size() >= 2) {
            quint16 nameLen = readLE16(resp.rawData, 0);
            if (resp.rawData.size() >= 2 + nameLen)
                setLabelOK(m_programNameLabel, QString::fromUtf8(resp.rawData.mid(2, nameLen)));
        }
        break;
    case 0x1D:
        if (resp.rawData.size() >= 4)
            setLabelOK(m_segmentLabel, "N" + QString::number(readLE32(resp.rawData, 0)));
        break;
    case 0x23:
        if (resp.rawData.size() >= 4)
            setLabelOK(m_execLineLabel, QString::number(readLE32(resp.rawData, 0)));
        break;
    case 0x1B:
        if (resp.rawData.size() >= 2) {
            quint16 count = readLE16(resp.rawData, 0);
            QStringList vals;
            int off = 2;
            for (quint16 i = 0; i < count && off + 2 <= resp.rawData.size(); ++i) {
                vals << "G" + QString::number(readLE16(resp.rawData, off));
                off += 2;
            }
            setLabelOK(m_gModalLabel, vals.isEmpty() ? "无" : vals.join(" "));
        }
        break;
    case 0x1C:
        if (resp.rawData.size() >= 2) {
            quint16 count = readLE16(resp.rawData, 0);
            QStringList vals;
            int off = 2;
            for (quint16 i = 0; i < count && off + 2 <= resp.rawData.size(); ++i) {
                vals << "M" + QString::number(readLE16(resp.rawData, off));
                off += 2;
            }
            setLabelOK(m_mModalLabel, vals.isEmpty() ? "无" : vals.join(" "));
        }
        break;
    }
}

// ========== Alarm Card ==========

void Gsk988RealtimeWidget::updateAlarmCard(const ParsedResponse& resp)
{
    if (!resp.isValid || resp.cmdCode != 0x81) {
        if (resp.cmdCode == 0x81 && !resp.isValid) {
            setLabelError(m_errorCountLabel, QString());
            resetLabel(m_warnCountLabel);
        }
        return;
    }
    if (resp.rawData.size() < 4) return;

    setLabelOK(m_errorCountLabel, QString::number(readLE16(resp.rawData, 0)));
    setLabelOK(m_warnCountLabel, QString::number(readLE16(resp.rawData, 2)));
}

// ========== HEX Display ==========

void Gsk988RealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();

    QString prefix = isSend ? "[TX]" : "[RX]";
    m_hexDisplay->append(QString("%1 %2").arg(prefix, hex.join(" ")));
}

// ========== Polling Control ==========

void Gsk988RealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    // Start first cycle immediately
    startCycle();
}

void Gsk988RealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_cycleActive = false;
}

// ========== Clear Data ==========

void Gsk988RealtimeWidget::clearData()
{
    resetLabel(m_runModeLabel);
    resetLabel(m_runStatusLabel);
    resetLabel(m_pauseLabel);
    resetLabel(m_feedProgLabel);
    resetLabel(m_feedActualLabel);
    resetLabel(m_feedOverrideLabel);
    resetLabel(m_spindleProgLabel);
    resetLabel(m_spindleActualLabel);
    resetLabel(m_spindleOverrideLabel);
    resetLabel(m_rapidOverrideLabel);
    resetLabel(m_manualOverrideLabel);
    resetLabel(m_handwheelOverrideLabel);
    for (int i = 0; i < 3; ++i) {
        resetLabel(m_machineCoord[i]);
        resetLabel(m_absCoord[i]);
        resetLabel(m_relCoord[i]);
        resetLabel(m_remainCoord[i]);
    }
    resetLabel(m_pieceCountLabel);
    resetLabel(m_runTimeLabel);
    resetLabel(m_cutTimeLabel);
    resetLabel(m_toolNoLabel);
    resetLabel(m_programNameLabel);
    resetLabel(m_segmentLabel);
    resetLabel(m_execLineLabel);
    resetLabel(m_gModalLabel);
    resetLabel(m_mModalLabel);
    resetLabel(m_errorCountLabel);
    resetLabel(m_warnCountLabel);
    m_hexDisplay->clear();
}
