#include "syntecrealtimewidget.h"
#include "syntecframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QFrame>
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

const SyntecRealtimeWidget::PollItem SyntecRealtimeWidget::pollItems[] = {
    {0x03, QByteArray()}, // PI_STATUS
    {0x02, QByteArray()}, // PI_MODE
    {0x01, QByteArray()}, // PI_PROGNAME
    {0x04, QByteArray()}, // PI_ALARM
    {0x05, QByteArray()}, // PI_EMG
    {0x10, QByteArray()}, // PI_DECPOINT
    {0x20, QByteArray()}, // PI_REL_COORD
    {0x21, QByteArray()}, // PI_MACH_COORD
    {0x0C, QByteArray()}, // PI_FEED_OVERRIDE
    {0x0D, QByteArray()}, // PI_SPINDLE_OVERRIDE
    {0x0E, QByteArray()}, // PI_SPINDLE_SPEED
    {0x0F, QByteArray()}, // PI_FEED_RATE_ORI
    {0x06, QByteArray()}, // PI_RUN_TIME
    {0x07, QByteArray()}, // PI_CUT_TIME
    {0x08, QByteArray()}, // PI_CYCLE_TIME
    {0x09, QByteArray()}, // PI_PRODUCTS
};

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

// ========== Constructor ==========

SyntecRealtimeWidget::SyntecRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_cycleActive(false)
    , m_decPoint(3)
{
    setupUI();

    m_cycleDelayTimer = new QTimer(this);
    m_cycleDelayTimer->setSingleShot(true);
    connect(m_cycleDelayTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRefreshCheck->isChecked())
            startCycle();
    });

    m_interPollTimer = new QTimer(this);
    m_interPollTimer->setSingleShot(true);
    m_interPollTimer->setInterval(100);
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

void SyntecRealtimeWidget::setLabelOK(QLabel* label, const QString& text)
{
    label->setText(text);
    label->setStyleSheet(VALUE_STYLE);
}

void SyntecRealtimeWidget::setLabelError(QLabel* label, const QString&)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

void SyntecRealtimeWidget::resetLabel(QLabel* label)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

// ========== UI Setup ==========

void SyntecRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);

    auto* controlBar = new QHBoxLayout;
    controlBar->setSpacing(8);
    m_autoRefreshCheck = new QCheckBox(QStringLiteral("自动刷新"));
    m_autoRefreshCheck->setChecked(true);
    controlBar->addWidget(m_autoRefreshCheck);
    controlBar->addWidget(new QLabel(QStringLiteral("间隔:")));
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItem(QStringLiteral("1 秒"), 1);
    m_intervalCombo->addItem(QStringLiteral("2 秒"), 2);
    m_intervalCombo->addItem(QStringLiteral("5 秒"), 5);
    m_intervalCombo->setCurrentIndex(1);
    controlBar->addWidget(m_intervalCombo);
    m_manualRefreshBtn = new QPushButton(QStringLiteral("手动刷新"));
    m_manualRefreshBtn->setFixedWidth(80);
    controlBar->addWidget(m_manualRefreshBtn);
    controlBar->addStretch();
    mainLayout->addLayout(controlBar);

    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: #f0f0f0; }");
    auto* scrollWidget = new QWidget;
    scrollWidget->setStyleSheet("background: #f0f0f0;");
    auto* grid = new QGridLayout(scrollWidget);
    grid->setSpacing(8);
    grid->setContentsMargins(6, 6, 6, 6);

    // Row 0: Status | Feed/Spindle | Time
    m_statusGroup = new QGroupBox(QStringLiteral("运行状态"));
    m_statusGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_statusLabel = makeValueLabel();
        m_modeLabel = makeValueLabel();
        m_progNameLabel = makeValueLabel();
        m_emgLabel = makeValueLabel();
        m_alarmLabel = makeValueLabel();
        m_productsLabel = makeValueLabel();
        l->addRow(QStringLiteral("运行状态:"), m_statusLabel);
        l->addRow(QStringLiteral("操作模式:"), m_modeLabel);
        l->addRow(QStringLiteral("程序名:"), m_progNameLabel);
        l->addRow(QStringLiteral("急停:"), m_emgLabel);
        l->addRow(QStringLiteral("报警:"), m_alarmLabel);
        l->addRow(QStringLiteral("工件数:"), m_productsLabel);
        m_statusGroup->setLayout(l);
    }

    m_feedGroup = new QGroupBox(QStringLiteral("进给/主轴"));
    m_feedGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_feedRateLabel = makeValueLabel();
        m_feedOverrideLabel = makeValueLabel();
        m_spindleSpeedLabel = makeValueLabel();
        m_spindleOverrideLabel = makeValueLabel();
        l->addRow(QStringLiteral("进给速度:"), m_feedRateLabel);
        l->addRow(QStringLiteral("进给倍率:"), m_feedOverrideLabel);
        l->addRow(QStringLiteral("主轴速度:"), m_spindleSpeedLabel);
        l->addRow(QStringLiteral("主轴倍率:"), m_spindleOverrideLabel);
        m_feedGroup->setLayout(l);
    }

    m_timeGroup = new QGroupBox(QStringLiteral("时间"));
    m_timeGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_runTimeLabel = makeValueLabel();
        m_cutTimeLabel = makeValueLabel();
        m_cycleTimeLabel = makeValueLabel();
        l->addRow(QStringLiteral("开机时间:"), m_runTimeLabel);
        l->addRow(QStringLiteral("切削时间:"), m_cutTimeLabel);
        l->addRow(QStringLiteral("循环时间:"), m_cycleTimeLabel);
        m_timeGroup->setLayout(l);
    }

    // Row 1: Coordinates (spans 3 cols)
    m_coordGroup = new QGroupBox(QStringLiteral("坐标"));
    m_coordGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QGridLayout;
        l->setSpacing(4);
        l->setContentsMargins(8, 16, 8, 8);
        l->addWidget(makeHeaderLabel(""), 0, 0);
        l->addWidget(makeHeaderLabel("X"), 0, 1);
        l->addWidget(makeHeaderLabel("Y"), 0, 2);
        l->addWidget(makeHeaderLabel("Z"), 0, 3);
        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #d0d0d0;");
        l->addWidget(sep, 1, 0, 1, 4);
        l->addWidget(makeRowLabel(QStringLiteral("机床坐标:")), 2, 0);
        l->addWidget(makeRowLabel(QStringLiteral("相对坐标:")), 3, 0);
        for (int i = 0; i < 3; ++i) {
            m_machCoord[i] = makeValueLabel();
            m_relCoord[i] = makeValueLabel();
            l->addWidget(m_machCoord[i], 2, i + 1);
            l->addWidget(m_relCoord[i], 3, i + 1);
        }
        m_coordGroup->setLayout(l);
    }

    grid->addWidget(m_statusGroup,  0, 0);
    grid->addWidget(m_feedGroup,    0, 1);
    grid->addWidget(m_timeGroup,    0, 2);
    grid->addWidget(m_coordGroup,   1, 0, 1, 3);

    for (int c = 0; c < 3; ++c)
        grid->setColumnStretch(c, 1);

    scrollArea->setWidget(scrollWidget);

    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(80);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");

    mainLayout->addWidget(scrollArea, 3);
    mainLayout->addWidget(m_hexDisplay, 1);
}

// ========== Cycle Polling ==========

void SyntecRealtimeWidget::startCycle()
{
    if (m_cycleActive) return;
    m_cycleActive = true;
    m_pollIndex = 0;
    const auto& item = pollItems[0];
    emit pollRequest(item.cmdCode, item.params);
}

// ========== Update Data ==========

void SyntecRealtimeWidget::updateData(const ParsedResponse& resp)
{
    if (!m_cycleActive) return;

    int currentIdx = m_pollIndex;
    const QByteArray& data = resp.rawData;
    using FB = SyntecFrameBuilder;

    if (resp.isValid) {
        switch (currentIdx) {
        case PI_STATUS: {
            if (data.size() >= 4) {
                int status = FB::extractInt32(data, 0);
                setLabelOK(m_statusLabel, FB::statusToString(status));
            }
            break;
        }
        case PI_MODE: {
            if (data.size() >= 4) {
                int mode = FB::extractInt32(data, 0);
                setLabelOK(m_modeLabel, FB::modeToString(mode));
            }
            break;
        }
        case PI_PROGNAME: {
            setLabelOK(m_progNameLabel, FB::extractString(data, 0));
            break;
        }
        case PI_ALARM: {
            if (data.size() >= 4) {
                int alarm = FB::extractInt32(data, 0);
                setLabelOK(m_alarmLabel, alarm > 0 ? QString::number(alarm) : QStringLiteral("无"));
            }
            break;
        }
        case PI_EMG: {
            if (data.size() >= 5) {
                int emg = static_cast<quint8>(data[4]);
                setLabelOK(m_emgLabel, FB::emgToString(emg));
            }
            break;
        }
        case PI_DECPOINT: {
            if (data.size() >= 4) {
                m_decPoint = FB::extractInt32(data, 0);
                if (m_decPoint < 0) m_decPoint = 3;
            }
            break;
        }
        case PI_REL_COORD: {
            for (int i = 0; i < 3 && i * 8 + 8 <= data.size(); ++i) {
                qint64 raw = FB::extractInt64(data, i * 8);
                double val = static_cast<double>(raw);
                for (int d = 0; d < m_decPoint; ++d) val /= 10.0;
                setLabelOK(m_relCoord[i], QString::number(val, 'f', qMax(m_decPoint, 1)));
            }
            break;
        }
        case PI_MACH_COORD: {
            for (int i = 0; i < 3 && i * 8 + 8 <= data.size(); ++i) {
                qint64 raw = FB::extractInt64(data, i * 8);
                double val = static_cast<double>(raw);
                for (int d = 0; d < m_decPoint; ++d) val /= 10.0;
                setLabelOK(m_machCoord[i], QString::number(val, 'f', qMax(m_decPoint, 1)));
            }
            break;
        }
        case PI_FEED_OVERRIDE: {
            if (data.size() >= 4) {
                setLabelOK(m_feedOverrideLabel, QString::number(FB::extractInt32(data, 0)) + "%");
            }
            break;
        }
        case PI_SPINDLE_OVERRIDE: {
            if (data.size() >= 4) {
                setLabelOK(m_spindleOverrideLabel, QString::number(FB::extractInt32(data, 0)) + "%");
            }
            break;
        }
        case PI_SPINDLE_SPEED: {
            if (data.size() >= 4) {
                setLabelOK(m_spindleSpeedLabel, QString::number(FB::extractInt32(data, 0)) + " RPM");
            }
            break;
        }
        case PI_FEED_RATE_ORI: {
            if (data.size() >= 4) {
                qint32 raw = FB::extractInt32(data, 0);
                double val = static_cast<double>(raw);
                for (int d = 0; d < m_decPoint; ++d) val /= 10.0;
                setLabelOK(m_feedRateLabel, QString::number(val, 'f', qMax(m_decPoint, 1)));
            }
            break;
        }
        case PI_RUN_TIME: {
            if (data.size() >= 4) {
                setLabelOK(m_runTimeLabel, FB::formatTime(FB::extractInt32(data, 0)));
            }
            break;
        }
        case PI_CUT_TIME: {
            if (data.size() >= 4) {
                setLabelOK(m_cutTimeLabel, FB::formatTime(FB::extractInt32(data, 0)));
            }
            break;
        }
        case PI_CYCLE_TIME: {
            if (data.size() >= 4) {
                setLabelOK(m_cycleTimeLabel, FB::formatTime(FB::extractInt32(data, 0)));
            }
            break;
        }
        case PI_PRODUCTS: {
            if (data.size() >= 4) {
                setLabelOK(m_productsLabel, QString::number(FB::extractInt32(data, 0)));
            }
            break;
        }
        }
    } else {
        switch (currentIdx) {
        case PI_STATUS:     setLabelError(m_statusLabel, QString()); break;
        case PI_MODE:       setLabelError(m_modeLabel, QString()); break;
        case PI_PROGNAME:   setLabelError(m_progNameLabel, QString()); break;
        case PI_ALARM:      setLabelError(m_alarmLabel, QString()); break;
        case PI_EMG:        setLabelError(m_emgLabel, QString()); break;
        case PI_DECPOINT:   break;
        case PI_REL_COORD:  for (int i = 0; i < 3; ++i) setLabelError(m_relCoord[i], QString()); break;
        case PI_MACH_COORD: for (int i = 0; i < 3; ++i) setLabelError(m_machCoord[i], QString()); break;
        case PI_FEED_OVERRIDE:    setLabelError(m_feedOverrideLabel, QString()); break;
        case PI_SPINDLE_OVERRIDE: setLabelError(m_spindleOverrideLabel, QString()); break;
        case PI_SPINDLE_SPEED:    setLabelError(m_spindleSpeedLabel, QString()); break;
        case PI_FEED_RATE_ORI:    setLabelError(m_feedRateLabel, QString()); break;
        case PI_RUN_TIME:   setLabelError(m_runTimeLabel, QString()); break;
        case PI_CUT_TIME:   setLabelError(m_cutTimeLabel, QString()); break;
        case PI_CYCLE_TIME: setLabelError(m_cycleTimeLabel, QString()); break;
        case PI_PRODUCTS:   setLabelError(m_productsLabel, QString()); break;
        }
    }

    m_pollIndex++;
    if (m_pollIndex >= PI_COUNT) {
        m_cycleActive = false;
        if (m_autoRefreshCheck->isChecked()) {
            int secs = m_intervalCombo->currentData().toInt();
            m_cycleDelayTimer->start(secs * 1000);
        }
    } else {
        m_interPollTimer->start();
    }
}

// ========== Polling Control ==========

void SyntecRealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    m_interPollTimer->stop();
    startCycle();
}

void SyntecRealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_interPollTimer->stop();
    m_cycleActive = false;
}

// ========== Clear Data ==========

void SyntecRealtimeWidget::clearData()
{
    resetLabel(m_statusLabel);
    resetLabel(m_modeLabel);
    resetLabel(m_progNameLabel);
    resetLabel(m_emgLabel);
    resetLabel(m_alarmLabel);
    resetLabel(m_productsLabel);
    resetLabel(m_feedRateLabel);
    resetLabel(m_feedOverrideLabel);
    resetLabel(m_spindleSpeedLabel);
    resetLabel(m_spindleOverrideLabel);
    resetLabel(m_runTimeLabel);
    resetLabel(m_cutTimeLabel);
    resetLabel(m_cycleTimeLabel);
    for (int i = 0; i < 3; ++i) {
        resetLabel(m_machCoord[i]);
        resetLabel(m_relCoord[i]);
    }
    m_hexDisplay->clear();
}

// ========== HEX Display ==========

void SyntecRealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
    m_hexDisplay->append(QString("[%1] %2").arg(isSend ? "TX" : "RX", hex.join(" ")));
}
