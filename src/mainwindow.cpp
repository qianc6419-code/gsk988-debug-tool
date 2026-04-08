#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "network/tcpclient.h"
#include "network/mockserver.h"
#include "protocol/framebuilder.h"
#include "protocol/gsk988protocol.h"
#include "ui/realtimewidget.h"
#include "ui/commandtablewidget.h"
#include "ui/dataparsewidget.h"
#include "ui/logwidget.h"
#include "ui/paramwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftPanel(nullptr)
    , m_tabWidget(nullptr)
    , m_ipEdit(nullptr)
    , m_portSpin(nullptr)
    , m_connectBtn(nullptr)
    , m_modeGroup(nullptr)
    , m_realDeviceRadio(nullptr)
    , m_mockServerRadio(nullptr)
    , m_connectionStatusLabel(nullptr)
    , m_btnReadStatus(nullptr)
    , m_btnReadCoords(nullptr)
    , m_btnReadAlarms(nullptr)
    , m_btnReadParams(nullptr)
    , m_btnReadDeviceInfo(nullptr)
    , m_btnReadDiagnose(nullptr)
    , m_tcpClient(nullptr)
    , m_mockServer(nullptr)
    , m_realtimeWidget(nullptr)
    , m_commandWidget(nullptr)
    , m_parseWidget(nullptr)
    , m_logWidget(nullptr)
    , m_paramWidget(nullptr)
    , m_isConnected(false)
    , m_isMockMode(true)
{
    // Create network instances
    m_tcpClient = new TcpClient(this);
    m_mockServer = new MockServer(this);

    // Create protocol instance (for parsing)
    Gsk988Protocol *protocol = new Gsk988Protocol(this);

    // Create UI
    setupUi();

    // Connect signals
    // MockServer signals -> logWidget
    connect(m_mockServer, &MockServer::mockDataSent, this, [this](const QByteArray &data) {
        m_logWidget->addLog("发送", "", data);
    });
    connect(m_mockServer, &MockServer::mockDataReceived, this, [this](const QByteArray &data) {
        m_logWidget->addLog("接收", "", data);
    });

    // TcpClient signals
    connect(m_tcpClient, &TcpClient::readyRead, this, [this, protocol](const QByteArray &data) {
        // Parse response and update widgets
        quint8 cmd = 0, sub = 0, type = 0;
        QByteArray payload;
        QString error;

        if (FrameBuilder::parseFrame(data, cmd, sub, type, payload, error)) {
            m_logWidget->addLog("接收", QString("CMD=%1 SUB=%2 TYPE=%3").arg(cmd, 2, 16).arg(sub, 2, 16).arg(type, 2, 16), data);

            QMap<QString, QVariant> result;
            if (protocol->parseResponse(data, cmd, result)) {
                if (cmd == 0x02) {
                    m_realtimeWidget->updateMachineStatus(result);
                } else if (cmd == 0x03) {
                    m_realtimeWidget->updateCoordinates(result);
                } else if (cmd == 0x05) {
                    m_realtimeWidget->updateAlarms(result);
                }
                m_realtimeWidget->appendRawData(data);
            }
        } else {
            qDebug() << "Frame parse error:" << error;
        }
    });

    connect(m_tcpClient, &TcpClient::connectionError, this, [](const QString &error) {
        qDebug() << "Connection error:" << error;
    });

    connect(m_tcpClient, &TcpClient::timeout, this, []() {
        qDebug() << "Connection timeout";
    });

    connect(m_tcpClient, &TcpClient::connected, this, [this]() {
        m_isConnected = true;
        updateConnectionStatus();
    });

    connect(m_tcpClient, &TcpClient::disconnected, this, [this]() {
        m_isConnected = false;
        updateConnectionStatus();
    });

    // Quick buttons
    connect(m_btnReadStatus, &QPushButton::clicked, this, [this]() {
        sendCommand(0x02, 0x00, 0x00);
    });
    connect(m_btnReadCoords, &QPushButton::clicked, this, [this]() {
        sendCommand(0x03, 0x00, 0x00);
    });
    connect(m_btnReadAlarms, &QPushButton::clicked, this, [this]() {
        sendCommand(0x05, 0x00, 0x00);
    });
    connect(m_btnReadParams, &QPushButton::clicked, this, [this]() {
        sendCommand(0x06, 0x00, 0x00);
    });
    connect(m_btnReadDeviceInfo, &QPushButton::clicked, this, [this]() {
        sendCommand(0x01, 0x00, 0x00);
    });
    connect(m_btnReadDiagnose, &QPushButton::clicked, this, [this]() {
        sendCommand(0x09, 0x00, 0x00);
    });

    // Command table widget signal
    connect(m_commandWidget, &CommandTableWidget::commandSendRequested, this, [this](quint8 cmd, quint8 sub, quint8 type, const QByteArray &data) {
        sendCommand(cmd, sub, type, data);
    });

    // Connect button
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    // Mode radio buttons
    connect(m_modeGroup, &QButtonGroup::buttonClicked, this, [this](int id) {
        onModeChanged(id);
    });

    // Start mock server on port 8067
    if (m_mockServer->start(8067)) {
        m_connectionStatusLabel->setText("Mock模式 (本地8067)");
        m_connectionStatusLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
    } else {
        m_connectionStatusLabel->setText("Mock服务器启动失败");
        m_connectionStatusLabel->setStyleSheet("color: #F44336; font-weight: bold;");
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    resize(1280, 800);
    setWindowTitle("GSK988 Debug Tool");

    // Central widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Main horizontal layout with splitter
    QHBoxLayout *mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);

    // Left panel (fixed 220px)
    m_leftPanel = new QWidget(m_mainSplitter);
    m_leftPanel->setFixedWidth(220);
    m_leftPanel->setStyleSheet("background-color: #F5F5F5; border-right: 1px solid #E0E0E0;");

    QVBoxLayout *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(15);

    // Connection Group
    QGroupBox *connGroup = new QGroupBox("连接", m_leftPanel);
    connGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 4px; margin-top: 8px; }");
    QVBoxLayout *connLayout = new QVBoxLayout(connGroup);

    // IP Edit
    m_ipEdit = new QLineEdit(connGroup);
    m_ipEdit->setPlaceholderText("192.168.1.100");
    m_ipEdit->setText("127.0.0.1");
    connLayout->addWidget(new QLabel("IP地址:", connGroup));
    connLayout->addWidget(m_ipEdit);

    // Port Spin
    m_portSpin = new QSpinBox(connGroup);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(8067);
    connLayout->addWidget(new QLabel("端口:", connGroup));
    connLayout->addWidget(m_portSpin);

    // Connect Button
    m_connectBtn = new QPushButton("连接", connGroup);
    m_connectBtn->setStyleSheet("QPushButton { background-color: #2D7D46; color: white; border: none; border-radius: 4px; padding: 8px; font-weight: bold; } QPushButton:hover { background-color: #33A058; }");
    connLayout->addWidget(m_connectBtn);

    leftLayout->addWidget(connGroup);

    // Mode Group
    QGroupBox *modeGroup = new QGroupBox("模式", m_leftPanel);
    modeGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 4px; margin-top: 8px; }");
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);

    m_modeGroup = new QButtonGroup(modeGroup);
    m_realDeviceRadio = new QRadioButton("真实设备", modeGroup);
    m_mockServerRadio = new QRadioButton("Mock服务器", modeGroup);
    m_mockServerRadio->setChecked(true);
    m_modeGroup->addButton(m_realDeviceRadio, 0);
    m_modeGroup->addButton(m_mockServerRadio, 1);

    modeLayout->addWidget(m_realDeviceRadio);
    modeLayout->addWidget(m_mockServerRadio);

    leftLayout->addWidget(modeGroup);

    // Quick Group
    QGroupBox *quickGroup = new QGroupBox("快捷操作", m_leftPanel);
    quickGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 4px; margin-top: 8px; }");
    QGridLayout *quickLayout = new QGridLayout(quickGroup);

    m_btnReadStatus = new QPushButton("读状态", quickGroup);
    m_btnReadCoords = new QPushButton("读坐标", quickGroup);
    m_btnReadAlarms = new QPushButton("读报警", quickGroup);
    m_btnReadParams = new QPushButton("读参数", quickGroup);
    m_btnReadDeviceInfo = new QPushButton("读设备信息", quickGroup);
    m_btnReadDiagnose = new QPushButton("诊断", quickGroup);

    quickLayout->addWidget(m_btnReadStatus, 0, 0);
    quickLayout->addWidget(m_btnReadCoords, 0, 1);
    quickLayout->addWidget(m_btnReadAlarms, 1, 0);
    quickLayout->addWidget(m_btnReadParams, 1, 1);
    quickLayout->addWidget(m_btnReadDeviceInfo, 2, 0);
    quickLayout->addWidget(m_btnReadDiagnose, 2, 1);

    leftLayout->addWidget(quickGroup);

    // Add stretch to push everything up
    leftLayout->addStretch();

    // Connection status label at bottom of left panel
    m_connectionStatusLabel = new QLabel("未连接", m_leftPanel);
    m_connectionStatusLabel->setAlignment(Qt::AlignCenter);
    m_connectionStatusLabel->setStyleSheet("color: #9E9E9E; font-weight: bold; padding: 5px;");
    leftLayout->addWidget(m_connectionStatusLabel);

    // Right side - Tab Widget
    m_tabWidget = new QTabWidget(m_mainSplitter);

    // Create tab widgets
    m_realtimeWidget = new RealtimeWidget(m_tabWidget);
    m_commandWidget = new CommandTableWidget(m_tabWidget);
    m_parseWidget = new DataParseWidget(m_tabWidget);
    m_logWidget = new LogWidget(m_tabWidget);
    m_paramWidget = new ParamWidget(m_tabWidget);

    // Populate command table
    m_commandWidget->populateCommands();

    // Add tabs
    m_tabWidget->addTab(m_realtimeWidget, "实时数据");
    m_tabWidget->addTab(m_commandWidget, "命令表");
    m_tabWidget->addTab(m_parseWidget, "数据解析");
    m_tabWidget->addTab(m_logWidget, "日志");
    m_tabWidget->addTab(m_paramWidget, "参数");

    // Set splitter sizes (left 220, right stretches)
    QList<int> sizes;
    sizes << 220 << width() - 220;
    m_mainSplitter->setSizes(sizes);
    m_mainSplitter->setStretchFactor(1, 1);
}

void MainWindow::onConnectClicked()
{
    if (m_isConnected) {
        // Disconnect
        if (m_isMockMode) {
            m_isConnected = false;
            updateConnectionStatus();
        } else {
            m_tcpClient->disconnect();
        }
    } else {
        // Connect
        if (m_isMockMode) {
            // Mock mode - just set connected flag, mock server already running
            m_isConnected = true;
            updateConnectionStatus();
        } else {
            // Real device mode
            QString ip = m_ipEdit->text();
            quint16 port = m_portSpin->value();
            if (m_tcpClient->connectTo(ip, port)) {
                m_isConnected = true;
                updateConnectionStatus();
            }
        }
    }
}

void MainWindow::onModeChanged(int id)
{
    if (id == 1) {
        // Mock mode selected
        m_isMockMode = true;
        m_isConnected = false;
        m_connectionStatusLabel->setText("Mock模式 (本地8067)");
        m_connectionStatusLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
        m_connectBtn->setText("连接");
    } else {
        // Real device mode
        m_isMockMode = false;
        m_isConnected = false;
        m_connectionStatusLabel->setText("未连接");
        m_connectionStatusLabel->setStyleSheet("color: #9E9E9E; font-weight: bold;");
        m_connectBtn->setText("连接");
    }
}

void MainWindow::sendCommand(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data)
{
    QByteArray frame = FrameBuilder::buildFrame(cmd, sub, type, data);

    if (m_isMockMode) {
        // In mock mode, just log the send - mock server responds on its own
        m_logWidget->addLog("发送", QString("CMD=%1 SUB=%2 TYPE=%3").arg(cmd, 2, 16).arg(sub, 2, 16).arg(type, 2, 16), frame);
    } else {
        // Real mode
        if (m_isConnected) {
            m_tcpClient->sendFrame(frame);
            m_logWidget->addLog("发送", QString("CMD=%1 SUB=%2 TYPE=%3").arg(cmd, 2, 16).arg(sub, 2, 16).arg(type, 2, 16), frame);
        } else {
            qDebug() << "Cannot send: not connected";
        }
    }
}

void MainWindow::updateConnectionStatus()
{
    if (m_isConnected) {
        if (m_isMockMode) {
            m_connectionStatusLabel->setText("Mock已连接");
            m_connectionStatusLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else {
            m_connectionStatusLabel->setText("已连接 " + m_ipEdit->text() + ":" + QString::number(m_portSpin->value()));
            m_connectionStatusLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        }
        m_connectBtn->setText("断开");
    } else {
        if (m_isMockMode) {
            m_connectionStatusLabel->setText("Mock模式 (本地8067)");
            m_connectionStatusLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
        } else {
            m_connectionStatusLabel->setText("未连接");
            m_connectionStatusLabel->setStyleSheet("color: #9E9E9E; font-weight: bold;");
        }
        m_connectBtn->setText("连接");
    }
}
