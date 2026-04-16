#include "siemensrealtimewidget.h"
#include "siemensframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>

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

const SiemensRealtimeWidget::PollItem SiemensRealtimeWidget::pollItems[] = {
    { 0x05, {} },  // PI_MODE
    { 0x06, {} },  // PI_STATUS
    { 0x07, {} },  // PI_SPINDLE_SET_SPEED
    { 0x08, {} },  // PI_SPINDLE_ACT_SPEED
    { 0x09, {} },  // PI_SPINDLE_RATE
    { 0x0A, {} },  // PI_FEED_SET_SPEED
    { 0x0B, {} },  // PI_FEED_ACT_SPEED
    { 0x0C, {} },  // PI_FEED_RATE
    { 0x2D, {} },  // PI_ALARM_NUM
    { 0x01, {} },  // PI_CNC_ID
    { 0x02, {} },  // PI_CNC_TYPE
    { 0x03, {} },  // PI_VER_INFO
    { 0x0D, {} },  // PI_PRODUCTS
    { 0x0E, {} },  // PI_SET_PRODUCTS
    { 0x0F, {} },  // PI_CYCLE_TIME
    { 0x10, {} },  // PI_REMAIN_TIME
    { 0x11, {} },  // PI_PROG_NAME
    { 0x13, {} },  // PI_TOOL_NUM
    { 0x14, {} },  // PI_TOOL_D
    { 0x15, {} },  // PI_TOOL_H
    { 0x16, {} },  // PI_TOOL_X_LEN
    { 0x17, {} },  // PI_TOOL_Z_LEN
    { 0x18, {} },  // PI_TOOL_RADIU
    { 0x19, {} },  // PI_TOOL_EDG
    { 0x23, {} },  // PI_AXIS_NAME
    { 0x1A, {} },  // PI_MACH_X
    { 0x1B, {} },  // PI_MACH_Y
    { 0x1C, {} },  // PI_MACH_Z
    { 0x1D, {} },  // PI_WORK_X
    { 0x1E, {} },  // PI_WORK_Y
    { 0x1F, {} },  // PI_WORK_Z
    { 0x20, {} },  // PI_REMAIN_X
    { 0x21, {} },  // PI_REMAIN_Y
    { 0x22, {} },  // PI_REMAIN_Z
    { 0x24, {} },  // PI_DRIVE_VOLTAGE
    { 0x25, {} },  // PI_DRIVE_CURRENT
    { 0x26, {} },  // PI_DRIVE_LOAD1
    { 0x27, {} },  // PI_SPINDLE_LOAD1
    { 0x28, {} },  // PI_SPINDLE_LOAD2
    { 0x2C, {} },  // PI_DRIVE_TEMPER
};
static_assert(sizeof(SiemensRealtimeWidget::pollItems) / sizeof(SiemensRealtimeWidget::PollItem)
              == SiemensRealtimeWidget::PI_COUNT, "pollItems count mismatch");

// ========== Constructor ==========

SiemensRealtimeWidget::SiemensRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_cycleActive(false)
    , m_alarmCount(0)
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
        if (!checked) {
            m_cycleDelayTimer->stop();
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

void SiemensRealtimeWidget::setLabelOK(QLabel* label, const QString& text)
{
    label->setText(text);
    label->setStyleSheet(VALUE_STYLE);
}

void SiemensRealtimeWidget::setLabelError(QLabel* label, const QString& /*err*/)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

void SiemensRealtimeWidget::resetLabel(QLabel* label)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

// ========== UI Setup ==========

void SiemensRealtimeWidget::setupUI()
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
        m_alarmIndicator = makeValueLabel();
        l->addRow("操作模式:", m_modeLabel);
        l->addRow("运行状态:", m_runStatusLabel);
        l->addRow("告警指示:", m_alarmIndicator);
        m_alarmIndicator->setText("--");
        m_alarmIndicator->setStyleSheet(
            "font-size: 13px; font-weight: bold; font-family: Consolas, 'Courier New', monospace;"
            "color: #888; padding: 2px 6px;"
            "background: #f5f6f8; border: 1px solid #dcdfe3; border-radius: 3px;");
        m_runStatusGroup->setLayout(l);
    }

    m_spindleGroup = new QGroupBox("主轴");
    m_spindleGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_spindleActSpeedLabel = makeValueLabel();
        m_spindleRateLabel = makeValueLabel();
        m_spindleSetSpeedLabel = makeValueLabel();
        l->addRow("实际速度:", m_spindleActSpeedLabel);
        l->addRow("倍率:", m_spindleRateLabel);
        l->addRow("设定速度:", m_spindleSetSpeedLabel);
        m_spindleGroup->setLayout(l);
    }

    m_feedGroup = new QGroupBox("进给");
    m_feedGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_feedActSpeedLabel = makeValueLabel();
        m_feedRateLabel = makeValueLabel();
        m_feedSetSpeedLabel = makeValueLabel();
        l->addRow("实际速度:", m_feedActSpeedLabel);
        l->addRow("倍率:", m_feedRateLabel);
        l->addRow("设定速度:", m_feedSetSpeedLabel);
        m_feedGroup->setLayout(l);
    }

    // ===== Row 1: 系统信息 | 计数与时间 | 程序与刀具 =====

    m_sysInfoGroup = new QGroupBox("系统信息");
    m_sysInfoGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_cncTypeLabel = makeValueLabel();
        m_verInfoLabel = makeValueLabel();
        m_cncIdLabel = makeValueLabel();
        l->addRow("CNC型号:", m_cncTypeLabel);
        l->addRow("版本号:", m_verInfoLabel);
        l->addRow("机床指纹:", m_cncIdLabel);
        m_sysInfoGroup->setLayout(l);
    }

    m_counterTimeGroup = new QGroupBox("计数与时间");
    m_counterTimeGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_productsLabel = makeValueLabel();
        m_setProductsLabel = makeValueLabel();
        m_cycleTimeLabel = makeValueLabel();
        m_remainTimeLabel = makeValueLabel();
        l->addRow("工件数:", m_productsLabel);
        l->addRow("设定工件数:", m_setProductsLabel);
        l->addRow("循环时间:", m_cycleTimeLabel);
        l->addRow("剩余时间:", m_remainTimeLabel);
        m_counterTimeGroup->setLayout(l);
    }

    m_progToolGroup = new QGroupBox("程序与刀具");
    m_progToolGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_progNameLabel = makeValueLabel();
        m_toolNumLabel = makeValueLabel();
        m_toolDLabel = makeValueLabel();
        m_toolHLabel = makeValueLabel();
        m_toolRadiuLabel = makeValueLabel();
        m_toolXLenLabel = makeValueLabel();
        m_toolZLenLabel = makeValueLabel();
        m_toolEdgLabel = makeValueLabel();
        l->addRow("程序名:", m_progNameLabel);
        l->addRow("刀具号T:", m_toolNumLabel);
        l->addRow("刀补D:", m_toolDLabel);
        l->addRow("刀补H:", m_toolHLabel);
        l->addRow("磨损半径:", m_toolRadiuLabel);
        l->addRow("长补X:", m_toolXLenLabel);
        l->addRow("长补Z:", m_toolZLenLabel);
        l->addRow("刀沿位置:", m_toolEdgLabel);
        m_progToolGroup->setLayout(l);
    }

    // ===== Row 2: 告警 | 坐标 | 驱动诊断 =====

    m_alarmGroup = new QGroupBox("告警");
    m_alarmGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QVBoxLayout;
        m_alarmTextLabel = new QLabel("无告警");
        m_alarmTextLabel->setWordWrap(true);
        m_alarmTextLabel->setStyleSheet(
            "font-size: 12px; color: #555; background: #fff; "
            "border: 1px solid #dcdfe3; border-radius: 3px; padding: 4px;");
        m_alarmTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_alarmTextLabel->setMinimumHeight(60);
        l->addWidget(m_alarmTextLabel);
        m_alarmGroup->setLayout(l);
    }

    m_coordGroup = new QGroupBox("坐标");
    m_coordGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QGridLayout;
        l->setSpacing(4);
        l->setContentsMargins(8, 16, 8, 8);
        // Header row — axis names from m_axisNameLabels (default X/Y/Z)
        m_axisNameLabels[0] = makeHeaderLabel("X");
        m_axisNameLabels[1] = makeHeaderLabel("Y");
        m_axisNameLabels[2] = makeHeaderLabel("Z");
        l->addWidget(makeHeaderLabel(""), 0, 0);
        l->addWidget(m_axisNameLabels[0], 0, 1);
        l->addWidget(m_axisNameLabels[1], 0, 2);
        l->addWidget(m_axisNameLabels[2], 0, 3);
        // Separator line
        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #d0d0d0;");
        l->addWidget(sep, 1, 0, 1, 4);
        // Data rows
        l->addWidget(makeRowLabel("机械坐标:"), 2, 0);
        l->addWidget(makeRowLabel("工件坐标:"), 3, 0);
        l->addWidget(makeRowLabel("剩余距离:"), 4, 0);
        for (int i = 0; i < 3; ++i) {
            m_posMach[i] = makeValueLabel();
            m_posWork[i] = makeValueLabel();
            m_posRemain[i] = makeValueLabel();
            l->addWidget(m_posMach[i], 2, i + 1);
            l->addWidget(m_posWork[i], 3, i + 1);
            l->addWidget(m_posRemain[i], 4, i + 1);
        }
        m_coordGroup->setLayout(l);
    }

    m_driveGroup = new QGroupBox("驱动诊断");
    m_driveGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_driveVoltageLabel = makeValueLabel();
        m_driveCurrentLabel = makeValueLabel();
        m_driveLoad1Label = makeValueLabel();
        m_spindleLoad1Label = makeValueLabel();
        m_spindleLoad2Label = makeValueLabel();
        m_driveTemperLabel = makeValueLabel();
        l->addRow("母线电压:", m_driveVoltageLabel);
        l->addRow("实际电流:", m_driveCurrentLabel);
        l->addRow("电机功率:", m_driveLoad1Label);
        l->addRow("负载S轴:", m_spindleLoad1Label);
        l->addRow("负载X轴:", m_spindleLoad2Label);
        l->addRow("电机温度:", m_driveTemperLabel);
        m_driveGroup->setLayout(l);
    }

    // ===== Row 3: PLC告警 (3-column span) =====

    m_plcAlarmGroup = new QGroupBox("PLC告警");
    m_plcAlarmGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QHBoxLayout;
        m_plcAlarmLabel = new QLabel("PLC未连接");
        m_plcAlarmLabel->setStyleSheet(
            "font-size: 12px; color: #888; padding: 4px;");
        l->addWidget(m_plcAlarmLabel);
        l->addStretch();
        m_plcAlarmGroup->setLayout(l);
    }

    // ===== Grid layout =====
    grid->addWidget(m_runStatusGroup,  0, 0);
    grid->addWidget(m_spindleGroup,    0, 1);
    grid->addWidget(m_feedGroup,       0, 2);
    grid->addWidget(m_sysInfoGroup,    1, 0);
    grid->addWidget(m_counterTimeGroup,1, 1);
    grid->addWidget(m_progToolGroup,   1, 2);
    grid->addWidget(m_alarmGroup,      2, 0);
    grid->addWidget(m_coordGroup,      2, 1);
    grid->addWidget(m_driveGroup,      2, 2);
    grid->addWidget(m_plcAlarmGroup,   3, 0, 1, 3);

    // Make columns equal width
    for (int c = 0; c < 3; ++c)
        grid->setColumnStretch(c, 1);

    scrollArea->setWidget(scrollWidget);

    // HEX display
    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(120);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");

    mainLayout->addWidget(scrollArea, 3);
    mainLayout->addWidget(m_hexDisplay, 1);
}

// ========== Cycle Polling ==========

void SiemensRealtimeWidget::startCycle()
{
    if (m_cycleActive) return;
    m_cycleActive = true;
    m_pollIndex = 0;
    const auto& item = pollItems[0];
    emit pollRequest(item.cmdCode, item.params);
}

// ========== Update Data ==========

void SiemensRealtimeWidget::updateData(const ParsedResponse& resp)
{
    if (!resp.isValid || m_pollIndex < 0 || m_pollIndex >= PI_COUNT)
        return;

    using FB = SiemensFrameBuilder;
    const QByteArray& data = resp.rawData;

    // Validate S7 return code (byte 21 should be 0xFF for success)
    if (!FB::isResponseOK(data)) {
        // S7 read error — advance to next poll item without updating
        goto advance;
    }

    switch (m_pollIndex) {
    // ===== Row 0 — 运行状态 =====
    case PI_MODE:
        setLabelOK(m_modeLabel, FB::parseMode(data));
        break;
    case PI_STATUS:
        setLabelOK(m_runStatusLabel, FB::parseStatus(data));
        break;
    case PI_SPINDLE_SET_SPEED:
        setLabelOK(m_spindleSetSpeedLabel, QString::number(FB::parseDouble(data), 'f', 0) + " RPM");
        break;
    case PI_SPINDLE_ACT_SPEED:
        setLabelOK(m_spindleActSpeedLabel, QString::number(FB::parseDouble(data), 'f', 0) + " RPM");
        break;
    case PI_SPINDLE_RATE:
        setLabelOK(m_spindleRateLabel, QString::number(FB::parseDouble(data), 'f', 0) + " %");
        break;
    case PI_FEED_SET_SPEED:
        setLabelOK(m_feedSetSpeedLabel, QString::number(FB::parseDouble(data), 'f', 0) + " mm/min");
        break;
    case PI_FEED_ACT_SPEED:
        setLabelOK(m_feedActSpeedLabel, QString::number(FB::parseDouble(data), 'f', 0) + " mm/min");
        break;
    case PI_FEED_RATE:
        setLabelOK(m_feedRateLabel, QString::number(FB::parseDouble(data), 'f', 0) + " %");
        break;
    case PI_ALARM_NUM: {
        m_alarmCount = FB::parseInt32(data);
        if (m_alarmCount <= 0) {
            setLabelOK(m_alarmIndicator, QStringLiteral("无告警"));
            m_alarmTextLabel->setText(QStringLiteral("无告警"));
            m_alarmTextLabel->setStyleSheet("color: green;");
            m_alarmMessages.clear();
        } else {
            setLabelOK(m_alarmIndicator, QString("有(%1)").arg(m_alarmCount));
            m_alarmIndicator->setStyleSheet("color: red; font-weight: bold;");
            // Alarm details queried via manual command, here just show count
        }
        break;
    }

    // ===== Row 1 — 系统信息 =====
    case PI_CNC_ID:
        setLabelOK(m_cncIdLabel, FB::parseString(data));
        break;
    case PI_CNC_TYPE:
        setLabelOK(m_cncTypeLabel, FB::parseString(data));
        break;
    case PI_VER_INFO:
        setLabelOK(m_verInfoLabel, FB::parseString(data));
        break;

    // ===== Row 1 — 计数时间 =====
    case PI_PRODUCTS:
        setLabelOK(m_productsLabel, QString::number(FB::parseDouble(data), 'f', 0));
        break;
    case PI_SET_PRODUCTS:
        setLabelOK(m_setProductsLabel, QString::number(FB::parseDouble(data), 'f', 0));
        break;
    case PI_CYCLE_TIME: {
        double secs = FB::parseDouble(data);
        int h = static_cast<int>(secs) / 3600;
        int mi = (static_cast<int>(secs) % 3600) / 60;
        int s = static_cast<int>(secs) % 60;
        setLabelOK(m_cycleTimeLabel, QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(mi, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
        break;
    }
    case PI_REMAIN_TIME: {
        double secs = FB::parseDouble(data);
        int h = static_cast<int>(secs) / 3600;
        int mi = (static_cast<int>(secs) % 3600) / 60;
        int s = static_cast<int>(secs) % 60;
        setLabelOK(m_remainTimeLabel, QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(mi, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
        break;
    }

    // ===== Row 1 — 程序与刀具 =====
    case PI_PROG_NAME:
        setLabelOK(m_progNameLabel, FB::parseString(data));
        break;
    case PI_TOOL_NUM:
        setLabelOK(m_toolNumLabel, "T" + QString::number(FB::parseInt16(data)));
        break;
    case PI_TOOL_D:
        setLabelOK(m_toolDLabel, "D" + QString::number(FB::parseInt16(data)));
        break;
    case PI_TOOL_H:
        setLabelOK(m_toolHLabel, "H" + QString::number(FB::parseInt16(data)));
        break;
    case PI_TOOL_X_LEN:
        setLabelOK(m_toolXLenLabel, QString::number(FB::parseDouble(data), 'f', 3));
        break;
    case PI_TOOL_Z_LEN:
        setLabelOK(m_toolZLenLabel, QString::number(FB::parseDouble(data), 'f', 3));
        break;
    case PI_TOOL_RADIU:
        setLabelOK(m_toolRadiuLabel, QString::number(FB::parseDouble(data), 'f', 3));
        break;
    case PI_TOOL_EDG:
        setLabelOK(m_toolEdgLabel, QString::number(FB::parseDouble(data), 'f', 0));
        break;

    // ===== Row 2 — 坐标 =====
    case PI_AXIS_NAME: {
        QString names = FB::parseString(data);
        QStringList parts = names.split(' ', QString::SkipEmptyParts);
        for (int i = 0; i < 3 && i < parts.size(); ++i) {
            m_axisNameLabels[i]->setText(parts[i]);
        }
        break;
    }
    case PI_MACH_X: case PI_MACH_Y: case PI_MACH_Z:
        setLabelOK(m_posMach[m_pollIndex - PI_MACH_X],
            QString::number(FB::parseDouble(data), 'f', 3));
        break;
    case PI_WORK_X: case PI_WORK_Y: case PI_WORK_Z:
        setLabelOK(m_posWork[m_pollIndex - PI_WORK_X],
            QString::number(FB::parseDouble(data), 'f', 3));
        break;
    case PI_REMAIN_X: case PI_REMAIN_Y: case PI_REMAIN_Z:
        setLabelOK(m_posRemain[m_pollIndex - PI_REMAIN_X],
            QString::number(FB::parseDouble(data), 'f', 3));
        break;

    // ===== Row 2 — 驱动诊断 =====
    case PI_DRIVE_VOLTAGE:
        setLabelOK(m_driveVoltageLabel, QString::number(FB::parseFloat(data), 'f', 1) + " V");
        break;
    case PI_DRIVE_CURRENT:
        setLabelOK(m_driveCurrentLabel, QString::number(FB::parseFloat(data), 'f', 1) + " A");
        break;
    case PI_DRIVE_LOAD1:
        setLabelOK(m_driveLoad1Label, QString::number(FB::parseFloat(data), 'f', 1) + " kW");
        break;
    case PI_SPINDLE_LOAD1:
        setLabelOK(m_spindleLoad1Label, QString::number(FB::parseFloat(data), 'f', 1) + " %");
        break;
    case PI_SPINDLE_LOAD2:
        setLabelOK(m_spindleLoad2Label, QString::number(FB::parseFloat(data), 'f', 1) + " %");
        break;
    case PI_DRIVE_TEMPER:
        setLabelOK(m_driveTemperLabel, QString::number(FB::parseFloat(data), 'f', 1) + " C");
        break;

    default:
        break;
    }

advance:
    // Advance to next poll item
    m_pollIndex++;
    if (m_pollIndex >= PI_COUNT) {
        // Cycle complete — start delay timer for next cycle
        m_pollIndex = 0;
        m_cycleActive = false;
        if (m_autoRefreshCheck->isChecked()) {
            int secs = m_intervalCombo->currentData().toInt();
            m_cycleDelayTimer->start(secs * 1000);
        }
    } else {
        // Emit next poll request directly (don't call startCycle which resets state)
        const auto& item = pollItems[m_pollIndex];
        emit pollRequest(item.cmdCode, item.params);
    }
}

// ========== Polling Control ==========

void SiemensRealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    startCycle();
}

void SiemensRealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_cycleActive = false;
}

// ========== Clear Data ==========

void SiemensRealtimeWidget::clearData()
{
    resetLabel(m_modeLabel);
    resetLabel(m_runStatusLabel);
    resetLabel(m_alarmIndicator);
    resetLabel(m_spindleActSpeedLabel);
    resetLabel(m_spindleRateLabel);
    resetLabel(m_spindleSetSpeedLabel);
    resetLabel(m_feedActSpeedLabel);
    resetLabel(m_feedRateLabel);
    resetLabel(m_feedSetSpeedLabel);
    resetLabel(m_cncTypeLabel);
    resetLabel(m_verInfoLabel);
    resetLabel(m_cncIdLabel);
    resetLabel(m_productsLabel);
    resetLabel(m_setProductsLabel);
    resetLabel(m_cycleTimeLabel);
    resetLabel(m_remainTimeLabel);
    resetLabel(m_progNameLabel);
    resetLabel(m_toolNumLabel);
    resetLabel(m_toolDLabel);
    resetLabel(m_toolHLabel);
    resetLabel(m_toolRadiuLabel);
    resetLabel(m_toolXLenLabel);
    resetLabel(m_toolZLenLabel);
    resetLabel(m_toolEdgLabel);
    m_alarmTextLabel->setText("无告警");
    for (int i = 0; i < 3; ++i) {
        m_axisNameLabels[i]->setText(QString(QChar('X' + i)));
        resetLabel(m_posMach[i]);
        resetLabel(m_posWork[i]);
        resetLabel(m_posRemain[i]);
    }
    resetLabel(m_driveVoltageLabel);
    resetLabel(m_driveCurrentLabel);
    resetLabel(m_driveLoad1Label);
    resetLabel(m_spindleLoad1Label);
    resetLabel(m_spindleLoad2Label);
    resetLabel(m_driveTemperLabel);
    m_plcAlarmLabel->setText("PLC未连接");
    m_alarmCount = 0;
    m_alarmMessages.clear();
    m_hexDisplay->clear();
}

// ========== HEX Display ==========

void SiemensRealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
    m_hexDisplay->append(QString("[%1] %2").arg(isSend ? "TX" : "RX", hex.join(" ")));
}
