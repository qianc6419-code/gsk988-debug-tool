#include "protocol/mazak/mazakrealtimewidget.h"
#include "protocol/mazak/mazakdllwrapper.h"
#include "protocol/mazak/mazakframebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFont>

MazakRealtimeWidget::MazakRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollTimer(new QTimer(this))
    , m_healthCheckCounter(0)
{
    setupUI();
    m_pollTimer->setInterval(500);
    connect(m_pollTimer, &QTimer::timeout, this, &MazakRealtimeWidget::onPollTimeout);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MazakRealtimeWidget::onPollTimeout);
}

void MazakRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);

    QFont boldFont;
    boldFont.setBold(true);
    QFont valueFont;
    valueFont.setPointSize(10);

    // === 第1行: 系统状态 ===
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

    // === 第2行: 主轴 + 进给 ===
    auto* spGroup = new QGroupBox("主轴 / 进给");
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

    makeSpItem("设定S:", m_setSpeedLabel);
    makeSpItem("实际S:", m_actSpeedLabel);
    makeSpItem("S负载:", m_spLoadLabel);
    makeSpItem("S倍率:", m_spOverrideLabel);
    makeSpItem("进给:", m_feedLabel);
    makeSpItem("F倍率:", m_feedOverrideLabel);
    makeSpItem("快移%:", m_rapidOverrideLabel);
    spLayout->addStretch();
    mainLayout->addWidget(spGroup);

    // === 第3行: 坐标 ===
    auto* coordGroup = new QGroupBox("坐标");
    auto* coordLayout = new QVBoxLayout(coordGroup);

    m_axisTable = new QTableWidget(0, 4);
    m_axisTable->setHorizontalHeaderLabels({"轴", "机械坐标", "绝对坐标", "剩余距离"});
    m_axisTable->horizontalHeader()->setStretchLastSection(true);
    m_axisTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_axisTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_axisTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_axisTable->setAlternatingRowColors(true);
    coordLayout->addWidget(m_axisTable);
    mainLayout->addWidget(coordGroup, 1);

    // === 底部信息栏 ===
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

    makeBtmItem("报警:", m_alarmLabel);
    m_alarmLabel->setMinimumWidth(200);
    makeBtmItem("刀号:", m_toolLabel);
    makeBtmItem("开机:", m_powerOnLabel);
    makeBtmItem("运行:", m_runTimeLabel);

    bottomLayout->addStretch();
    m_refreshBtn = new QPushButton("刷新");
    bottomLayout->addWidget(m_refreshBtn);
    mainLayout->addWidget(bottomGroup);
}

void MazakRealtimeWidget::startPolling()
{
    m_pollTimer->start();
}

void MazakRealtimeWidget::stopPolling()
{
    m_pollTimer->stop();
}

void MazakRealtimeWidget::clearData()
{
    m_statusLabel->setText("--");
    m_modeLabel->setText("--");
    m_programLabel->setText("--");
    m_partsLabel->setText("--");
    m_setSpeedLabel->setText("--");
    m_actSpeedLabel->setText("--");
    m_spLoadLabel->setText("--");
    m_spOverrideLabel->setText("--");
    m_feedLabel->setText("--");
    m_feedOverrideLabel->setText("--");
    m_rapidOverrideLabel->setText("--");
    m_alarmLabel->setText("--");
    m_toolLabel->setText("--");
    m_powerOnLabel->setText("--");
    m_runTimeLabel->setText("--");
    m_axisTable->setRowCount(0);
}

void MazakRealtimeWidget::onPollTimeout()
{
    auto& dll = MazakDLLWrapper::instance();
    if (!dll.isConnected()) {
        stopPolling();
        return;
    }

    // 每秒进行一次健康检查（每2个poll周期）
    m_healthCheckCounter++;
    if (m_healthCheckCounter >= 2) {
        m_healthCheckCounter = 0;

        // 执行健康检查
        if (!dll.healthCheck()) {
            qWarning() << "Mazak 连接健康检查失败";
            clearData();
            stopPolling();
            emit disconnected();  // 通知上层连接断开
            return;
        }
    }

    // Mock 模式：DLL 未加载时显示模拟数据
    if (!dll.isLoaded()) {
        m_statusLabel->setText("运行");
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        m_modeLabel->setText("自动");
        m_programLabel->setText("O1234");
        m_partsLabel->setText("5");
        m_toolLabel->setText("T05");
        m_setSpeedLabel->setText("3000");
        m_actSpeedLabel->setText("2980");
        m_spLoadLabel->setText("45.0%");
        m_spOverrideLabel->setText("100%");
        m_feedLabel->setText("120.0");
        m_feedOverrideLabel->setText("100%");
        m_rapidOverrideLabel->setText("100%");
        m_alarmLabel->setText("无");
        m_alarmLabel->setStyleSheet("color: green;");
        m_powerOnLabel->setText("12:30:00");
        m_runTimeLabel->setText("05:20:00");

        // 模拟坐标表
        static const QStringList axes = {"X", "Y", "Z", "A"};
        static const QStringList mcPos = {"100.000", "200.000", "150.000", "0.000"};
        static const QStringList absPos = {"50.000", "80.000", "30.000", "0.000"};
        static const QStringList remPos = {"10.000", "5.000", "2.000", "0.000"};
        m_axisTable->setRowCount(axes.size());
        for (int i = 0; i < axes.size(); ++i) {
            if (!m_axisTable->item(i, 0)) {
                for (int c = 0; c < 4; ++c) {
                    auto* item = new QTableWidgetItem;
                    item->setTextAlignment(Qt::AlignCenter);
                    m_axisTable->setItem(i, c, item);
                }
            }
            m_axisTable->item(i, 0)->setText(axes[i]);
            m_axisTable->item(i, 1)->setText(mcPos[i]);
            m_axisTable->item(i, 2)->setText(absPos[i]);
            m_axisTable->item(i, 3)->setText(remPos[i]);
        }
        return;
    }

    // 真实模式：调用 DLL

    // 运行信息
    MTC_RUNNING_INFO ri;
    if (dll.getRunningInfo(ri)) {
        updateStatusLabel(ri.run_info[0]);
        updateModeLabel(ri.run_info[0]);
        m_partsLabel->setText(QString::number(ri.run_info[0].prtcnt));
        m_toolLabel->setText(QString("T%1").arg(ri.run_info[0].tno));
        m_spOverrideLabel->setText(QString("%1%").arg(ri.run_info[0].sp_override));
        m_feedOverrideLabel->setText(QString("%1%").arg(ri.run_info[0].ax_override));
        m_rapidOverrideLabel->setText(QString("%1%").arg(ri.run_info[0].rpid_override));
    }

    // 轴信息
    MTC_AXIS_INFO ai;
    if (dll.getAxisInfo(ai)) {
        updateAxisTable(ai);
    }

    // 主轴信息
    MTC_SPINDLE_INFO si;
    if (dll.getSpindleInfo(si)) {
        updateSpindleLabels(si);
    }

    // 设定转速
    int setRev = 0;
    if (dll.getOrderSpindleRev(0, setRev)) {
        m_setSpeedLabel->setText(QString::number(setRev));
    }

    // 进给
    MAZ_FEED feed;
    if (dll.getFeed(0, feed)) {
        m_feedLabel->setText(QString::number(feed.fmin / 10000.0, 'f', 1));
    }

    // 报警
    MAZ_ALARMALL al;
    if (dll.getAlarm(al)) {
        updateAlarmLabel(al);
    }

    // 程序
    MAZ_PROINFO pro;
    if (dll.getMainPro(0, pro)) {
        m_programLabel->setText(MazakFrameBuilder::fixedString(pro.wno, 33));
    }

    // 时间
    MAZ_ACCUM_TIME at;
    if (dll.getAccumulatedTime(0, at)) {
        updateTimeLabels(at);
    }
}

void MazakRealtimeWidget::updateStatusLabel(const MTC_ONE_RUNNING_INFO& ri)
{
    // 从 C# MazakTCPLib 移植的状态判断逻辑
    QString status;
    auto almno = ri.almno;
    auto ncsts = ri.ncsts;

    if (almno != 0 && almno != 1370 && almno != 468 && almno != 72 && almno != 82) {
        status = "报警";
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        QSet<qint16> runSts = {4111, 15, 1807, 1551, 1039, 1295, 1815, 1443, 271};
        QSet<qint16> idleSts = {5647, 5903, 1955, 259, 1283, 1795, 1827, 5891};
        if (runSts.contains(ncsts)) {
            status = "运行";
            m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else if (idleSts.contains(ncsts)) {
            status = "待机";
            m_statusLabel->setStyleSheet("color: #2196F3; font-weight: bold;");
        } else {
            status = QString("未知(%1)").arg(ncsts);
            m_statusLabel->setStyleSheet("");
        }
    }
    m_statusLabel->setText(status);
}

void MazakRealtimeWidget::updateModeLabel(const MTC_ONE_RUNNING_INFO& ri)
{
    QString mode;
    switch (ri.ncmode) {
    case 1:    mode = "快速"; break;
    case 2:    mode = "手动"; break;
    case 16:   mode = "回零"; break;
    case 256:  mode = "自动"; break;
    case 512:  mode = "Tape"; break;
    case 2048: mode = "MDI"; break;
    default:   mode = QString::number(ri.ncmode); break;
    }
    m_modeLabel->setText(mode);
}

void MazakRealtimeWidget::updateAxisTable(const MTC_AXIS_INFO& ai)
{
    // 获取剩余距离
    MAZ_NCPOS rem;
    bool hasRemain = MazakDLLWrapper::instance().getRemain(rem);

    int row = 0;
    for (int i = 0; i < 16; ++i) {
        QString name = MazakFrameBuilder::fixedString(ai.axis[i].axis_name, 4);
        if (name.isEmpty()) continue;

        if (m_axisTable->rowCount() <= row) {
            m_axisTable->insertRow(row);
            for (int c = 0; c < 4; ++c) {
                auto* item = new QTableWidgetItem;
                item->setTextAlignment(Qt::AlignCenter);
                m_axisTable->setItem(row, c, item);
            }
        }

        m_axisTable->item(row, 0)->setText(name);
        m_axisTable->item(row, 1)->setText(QString::number(ai.axis[i].mc_pos, 'f', 3));
        m_axisTable->item(row, 2)->setText(QString::number(ai.axis[i].pos, 'f', 3));

        if (hasRemain && i < 16) {
            m_axisTable->item(row, 3)->setText(QString::number(rem.data[i] / 10000.0, 'f', 3));
        } else {
            m_axisTable->item(row, 3)->setText("--");
        }
        ++row;
    }
    // 删除多余行
    while (m_axisTable->rowCount() > row) {
        m_axisTable->removeRow(m_axisTable->rowCount() - 1);
    }
}

void MazakRealtimeWidget::updateSpindleLabels(const MTC_SPINDLE_INFO& si)
{
    if (si.sp_info[0].rot > 0) {
        m_actSpeedLabel->setText(QString::number(si.sp_info[0].rot, 'f', 0));
    }
    m_spLoadLabel->setText(QString::number(si.sp_info[0].load, 'f', 1) + "%");
}

void MazakRealtimeWidget::updateAlarmLabel(const MAZ_ALARMALL& al)
{
    QString alarmText;
    for (int i = 0; i < 24; ++i) {
        if (al.alarm[i].eno == 0) continue;
        // 跳过屏蔽报警号
        if (al.alarm[i].eno == 72 || al.alarm[i].eno == 82 ||
            al.alarm[i].eno == 468 || al.alarm[i].eno == 1370) continue;

        if (!alarmText.isEmpty()) alarmText += "; ";
        alarmText += QString::number(al.alarm[i].eno) + ":" +
                     MazakFrameBuilder::fixedString(al.alarm[i].message, 64);
    }
    if (alarmText.isEmpty()) {
        alarmText = "无";
        m_alarmLabel->setStyleSheet("color: green;");
    } else {
        m_alarmLabel->setStyleSheet("color: red; font-weight: bold;");
    }
    m_alarmLabel->setText(alarmText);
}

void MazakRealtimeWidget::updateTimeLabels(const MAZ_ACCUM_TIME& at)
{
    m_powerOnLabel->setText(QString("%1:%2:%3")
        .arg(at.power_on.hor, 2, 10, QChar('0'))
        .arg(at.power_on.min, 2, 10, QChar('0'))
        .arg(at.power_on.sec, 2, 10, QChar('0')));

    m_runTimeLabel->setText(QString("%1:%2:%3")
        .arg(at.total_cut.hor, 2, 10, QChar('0'))
        .arg(at.total_cut.min, 2, 10, QChar('0'))
        .arg(at.total_cut.sec, 2, 10, QChar('0')));
}
