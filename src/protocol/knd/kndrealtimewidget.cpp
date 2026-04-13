#include "protocol/knd/kndrealtimewidget.h"
#include "protocol/knd/kndrestclient.h"
#include "protocol/knd/kndframebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFont>

KndRealtimeWidget::KndRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollTimer(new QTimer(this))
{
    setupUI();
    connectClientSignals();
    m_pollTimer->setInterval(500);
    connect(m_pollTimer, &QTimer::timeout, this, &KndRealtimeWidget::onPollTimeout);
    connect(m_refreshBtn, &QPushButton::clicked, this, &KndRealtimeWidget::onPollTimeout);
}

void KndRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);

    QFont boldFont;
    boldFont.setBold(true);
    QFont valueFont;
    valueFont.setPointSize(10);

    auto* sysGroup = new QGroupBox("系统状态");
    auto* sysLayout = new QHBoxLayout(sysGroup);

    auto makeItem = [&](const QString& title, QLabel*& label) {
        auto* l = new QLabel(title);
        l->setFont(boldFont);
        label = new QLabel("--");
        label->setFont(valueFont);
        label->setMinimumWidth(80);
        sysLayout->addWidget(l);
        sysLayout->addWidget(label);
    };

    makeItem("状态:", m_statusLabel);
    makeItem("模式:", m_modeLabel);
    makeItem("程序:", m_programLabel);
    m_programLabel->setMinimumWidth(120);
    makeItem("件数:", m_partsLabel);
    sysLayout->addStretch();
    mainLayout->addWidget(sysGroup);

    auto* spGroup = new QGroupBox("主轴 / 进给 / 倍率");
    auto* spLayout = new QHBoxLayout(spGroup);

    auto makeSpItem = [&](const QString& title, QLabel*& label) {
        auto* l = new QLabel(title);
        l->setFont(boldFont);
        label = new QLabel("--");
        label->setFont(valueFont);
        label->setMinimumWidth(70);
        spLayout->addWidget(l);
        spLayout->addWidget(label);
    };

    makeSpItem("进给:", m_feedLabel);
    makeSpItem("F%:", m_feedOverrideLabel);
    makeSpItem("快移%:", m_rapidOverrideLabel);
    makeSpItem("主轴:", m_spindleSpeedLabel);
    makeSpItem("S%:", m_spindleOverrideLabel);
    spLayout->addStretch();
    mainLayout->addWidget(spGroup);

    auto* coordGroup = new QGroupBox("坐标");
    auto* coordLayout = new QVBoxLayout(coordGroup);

    m_axisTable = new QTableWidget(0, 5);
    m_axisTable->setHorizontalHeaderLabels({"轴", "机床坐标", "绝对坐标", "相对坐标", "余移动量"});
    m_axisTable->horizontalHeader()->setStretchLastSection(true);
    m_axisTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_axisTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_axisTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_axisTable->setAlternatingRowColors(true);
    coordLayout->addWidget(m_axisTable);
    mainLayout->addWidget(coordGroup, 1);

    auto* bottomGroup = new QGroupBox("信息");
    auto* bottomLayout = new QHBoxLayout(bottomGroup);

    auto makeBtmItem = [&](const QString& title, QLabel*& label) {
        auto* l = new QLabel(title);
        l->setFont(boldFont);
        label = new QLabel("--");
        label->setFont(valueFont);
        bottomLayout->addWidget(l);
        bottomLayout->addWidget(label);
    };

    makeBtmItem("循环时间:", m_cycleTimeLabel);
    makeBtmItem("加工时间:", m_totalTimeLabel);
    makeBtmItem("驱动负载:", m_axisLoadLabel);
    m_axisLoadLabel->setMinimumWidth(200);

    bottomLayout->addStretch();
    m_refreshBtn = new QPushButton("刷新");
    bottomLayout->addWidget(m_refreshBtn);
    mainLayout->addWidget(bottomGroup);
}

void KndRealtimeWidget::connectClientSignals()
{
    auto& client = KndRestClient::instance();

    connect(&client, &KndRestClient::statusReceived, this, [this](const QJsonObject& data) {
        int runStatus = data["run-status"].toInt();
        m_statusLabel->setText(KndFrameBuilder::runStatusToString(runStatus));
        m_statusLabel->setStyleSheet(KndFrameBuilder::runStatusColor(runStatus));

        int mode = data["opr-mode"].toInt();
        m_modeLabel->setText(KndFrameBuilder::oprModeToString(mode));

        m_programLabel->setText(QString("O%1").arg(data["exec-no"].toInt()));

        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::coordinatesReceived, this, [this](const QJsonObject& data) {
        auto coords = KndFrameBuilder::parseCoordinates(data);
        m_axisTable->setRowCount(coords.size());
        for (int i = 0; i < coords.size(); ++i) {
            auto setItem = [&](int col, const QString& text) {
                if (!m_axisTable->item(i, col)) {
                    auto* item = new QTableWidgetItem;
                    item->setTextAlignment(Qt::AlignCenter);
                    m_axisTable->setItem(i, col, item);
                }
                m_axisTable->item(i, col)->setText(text);
            };
            setItem(0, coords[i].name);
            setItem(1, QString::number(coords[i].machine, 'f', 3));
            setItem(2, QString::number(coords[i].absolute, 'f', 3));
            setItem(3, QString::number(coords[i].relative, 'f', 3));
            setItem(4, QString::number(coords[i].distanceToGo, 'f', 3));
        }
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::overrideReceived, this,
        [this](const QString& type, const QJsonObject& data) {
        int ov = KndFrameBuilder::parseOverride(data);
        if (type == "feed") {
            m_feedOverrideLabel->setText(QString("%1%").arg(ov));
        } else if (type == "rapid") {
            m_rapidOverrideLabel->setText(QString("%1%").arg(ov));
        } else if (type == "spindle") {
            m_spindleOverrideLabel->setText(QString("%1%").arg(ov));
        }
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::spindleSpeedsReceived, this, [this](const QJsonObject& data) {
        auto speeds = KndFrameBuilder::parseSpindleSpeeds(data);
        if (!speeds.isEmpty()) {
            m_spindleSpeedLabel->setText(
                QString("S%1/%2").arg(speeds[0].setSpeed, 0, 'f', 0)
                                 .arg(speeds[0].actSpeed, 0, 'f', 0));
        }
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::feedRateReceived, this, [this](const QJsonObject& data) {
        m_feedLabel->setText(QString::number(data["feed-rate"].toDouble(), 'f', 1));
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::cycleTimeReceived, this, [this](const QJsonObject& data) {
        auto ct = KndFrameBuilder::parseCycleTime(data);
        m_cycleTimeLabel->setText(QString("%1:%2:%3")
            .arg(ct.cycleHour, 2, 10, QChar('0'))
            .arg(ct.cycleMin, 2, 10, QChar('0'))
            .arg(ct.cycleSec, 2, 10, QChar('0')));
        m_totalTimeLabel->setText(QString("%1:%2:%3")
            .arg(ct.totalHour, 2, 10, QChar('0'))
            .arg(ct.totalMin, 2, 10, QChar('0'))
            .arg(ct.totalSec, 2, 10, QChar('0')));
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::workCountsReceived, this, [this](const QJsonObject& data) {
        m_partsLabel->setText(QString::number(data["total"].toInt()));
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::axisLoadReceived, this, [this](const QJsonObject& data) {
        auto loads = KndFrameBuilder::parseAxisLoad(data);
        QString loadStr;
        for (const auto& l : loads) {
            if (!loadStr.isEmpty()) loadStr += "  ";
            loadStr += QString("%1:%2%").arg(l.name).arg(l.load, 0, 'f', 1);
        }
        m_axisLoadLabel->setText(loadStr.isEmpty() ? "--" : loadStr);
        m_pollBusy = false;
        startNextRequest();
    });

    connect(&client, &KndRestClient::errorOccurred, this, [this](const QString&) {
        m_pollBusy = false;
    });
}

void KndRealtimeWidget::startPolling()
{
    m_pollStep = 0;
    m_pollBusy = false;
    onPollTimeout();
    m_pollTimer->start();
}

void KndRealtimeWidget::stopPolling()
{
    m_pollTimer->stop();
}

void KndRealtimeWidget::clearData()
{
    m_statusLabel->setText("--");
    m_statusLabel->setStyleSheet("");
    m_modeLabel->setText("--");
    m_programLabel->setText("--");
    m_partsLabel->setText("--");
    m_feedLabel->setText("--");
    m_feedOverrideLabel->setText("--");
    m_rapidOverrideLabel->setText("--");
    m_spindleSpeedLabel->setText("--");
    m_spindleOverrideLabel->setText("--");
    m_cycleTimeLabel->setText("--");
    m_totalTimeLabel->setText("--");
    m_axisLoadLabel->setText("--");
    m_axisTable->setRowCount(0);
}

void KndRealtimeWidget::onPollTimeout()
{
    auto& client = KndRestClient::instance();
    if (!client.isConnected()) {
        stopPolling();
        return;
    }

    if (client.isMockMode()) {
        m_statusLabel->setText("运行");
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        m_modeLabel->setText("自动");
        m_programLabel->setText("O1234");
        m_partsLabel->setText("5");
        m_feedLabel->setText("120.0");
        m_feedOverrideLabel->setText("100%");
        m_rapidOverrideLabel->setText("100%");
        m_spindleSpeedLabel->setText("S3000/2980");
        m_spindleOverrideLabel->setText("100%");
        m_cycleTimeLabel->setText("00:05:30");
        m_totalTimeLabel->setText("12:20:00");
        m_axisLoadLabel->setText("X:45.0%  Y:32.0%  Z:28.0%");

        static const QStringList axes = {"X", "Y", "Z"};
        static const QStringList mcPos = {"100.000", "200.000", "150.000"};
        static const QStringList absPos = {"50.000", "80.000", "30.000"};
        static const QStringList relPos = {"20.000", "10.000", "5.000"};
        static const QStringList dtgPos = {"5.000", "2.000", "1.000"};
        m_axisTable->setRowCount(axes.size());
        for (int i = 0; i < axes.size(); ++i) {
            if (!m_axisTable->item(i, 0)) {
                for (int c = 0; c < 5; ++c) {
                    auto* item = new QTableWidgetItem;
                    item->setTextAlignment(Qt::AlignCenter);
                    m_axisTable->setItem(i, c, item);
                }
            }
            m_axisTable->item(i, 0)->setText(axes[i]);
            m_axisTable->item(i, 1)->setText(mcPos[i]);
            m_axisTable->item(i, 2)->setText(absPos[i]);
            m_axisTable->item(i, 3)->setText(relPos[i]);
            m_axisTable->item(i, 4)->setText(dtgPos[i]);
        }
        return;
    }

    m_pollStep = 0;
    m_pollBusy = false;
    startNextRequest();
}

void KndRealtimeWidget::startNextRequest()
{
    if (m_pollBusy) return;

    auto& client = KndRestClient::instance();
    if (!client.isConnected()) return;

    m_pollBusy = true;
    switch (m_pollStep) {
    case 0: client.getStatus(); break;
    case 1: client.getCoordinates(); break;
    case 2: client.getOverrideFeed(); break;
    case 3: client.getOverrideRapid(); break;
    case 4: client.getSpindleSpeeds(); break;
    case 5: client.getFeedRate(); break;
    case 6: client.getOverrideSpindle(1); break;
    case 7: client.getCycleTime(); break;
    case 8: client.getWorkCounts(); break;
    case 9:
        client.getAxisLoad();
        m_pollStep = 0;
        m_pollBusy = false;
        break;
    default:
        m_pollStep = 0;
        m_pollBusy = false;
        break;
    }
    if (m_pollStep <= 8) {
        ++m_pollStep;
    }
}