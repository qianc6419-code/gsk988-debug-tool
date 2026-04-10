#include "mainwindow.h"
#include "network/tcpclient.h"
#include "network/mockserver.h"
#include "protocol/gsk988protocol.h"
#include "ui/realtimewidget.h"
#include "ui/commandwidget.h"
#include "ui/parsewidget.h"
#include "ui/logwidget.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_protocol(new Gsk988Protocol(this))
    , m_client(new TcpClient(this))
    , m_mockServer(new MockServer(this))
    , m_waitingManualResponse(false)
    , m_needStartPolling(false)
{
    setWindowTitle("GSK988 以太网调试工具");
    resize(1100, 700);
    setMinimumSize(900, 600);

    setupToolBar();
    setupTabs();
    setupConnections();
    updateConnectionIndicator(Disconnected);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupToolBar()
{
    auto* toolbar = addToolBar("工具栏");
    toolbar->setMovable(false);

    // IP
    toolbar->addWidget(new QLabel(" IP: "));
    m_ipEdit = new QLineEdit("192.168.1.100");
    m_ipEdit->setFixedWidth(130);
    toolbar->addWidget(m_ipEdit);

    // Port
    toolbar->addWidget(new QLabel(" 端口: "));
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(6000);
    m_portSpin->setFixedWidth(80);
    toolbar->addWidget(m_portSpin);

    toolbar->addSeparator();

    // Mode
    m_modeCombo = new QComboBox;
    m_modeCombo->addItems({"真实设备", "Mock模式"});
    toolbar->addWidget(m_modeCombo);

    toolbar->addSeparator();

    // Connect button
    m_connectBtn = new QPushButton("连接");
    m_connectBtn->setFixedWidth(70);
    toolbar->addWidget(m_connectBtn);

    // Status indicator
    m_statusIndicator = new QLabel;
    m_statusIndicator->setFixedSize(14, 14);
    m_statusIndicator->setStyleSheet("background: gray; border-radius: 7px;");
    toolbar->addWidget(m_statusIndicator);

    m_statusLabel = new QLabel("未连接");
    toolbar->addWidget(m_statusLabel);
}

void MainWindow::setupTabs()
{
    m_tabWidget = new QTabWidget;
    setCentralWidget(m_tabWidget);

    m_realtimeTab = new RealtimeWidget;
    m_commandTab = new CommandWidget;
    m_parseTab = new ParseWidget;
    m_logTab = new LogWidget;

    m_tabWidget->addTab(m_realtimeTab, "实时数据");
    m_tabWidget->addTab(m_commandTab, "发送指令");
    m_tabWidget->addTab(m_parseTab, "数据解析");
    m_tabWidget->addTab(m_logTab, "通讯日志");
}

void MainWindow::setupConnections()
{
    // Connect button
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    // Client signals
    connect(m_client, &TcpClient::connected, this, [this]() {
        updateConnectionIndicator(Connected);
        m_connectBtn->setText("断开");
        m_statusLabel->setText("已连接");
        // Send permission request first; polling starts after 0x0A response
        m_needStartPolling = true;
        QByteArray permFrame = m_protocol->buildRequest(0x0A);
        m_client->sendFrame(permFrame);
    });

    connect(m_client, &TcpClient::disconnected, this, [this]() {
        updateConnectionIndicator(Disconnected);
        m_connectBtn->setText("连接");
        m_statusLabel->setText("未连接");
        m_realtimeTab->stopPolling();
        m_realtimeTab->clearData();
    });

    connect(m_client, &TcpClient::connectionError, this, [this](const QString& msg) {
        updateConnectionIndicator(Error);
        m_statusLabel->setText("连接失败");
        m_logTab->logError(msg);
    });

    connect(m_client, &TcpClient::responseReceived, this, &MainWindow::onResponseReceived);
    connect(m_client, &TcpClient::responseTimeout, this, &MainWindow::onResponseTimeout);

    // Realtime polling
    connect(m_realtimeTab, &RealtimeWidget::pollRequest, this, [this](quint8 cmdCode, const QByteArray& params) {
        QByteArray frame = m_protocol->buildRequest(cmdCode, params);
        m_client->sendFrame(frame);

        auto cmd = Gsk988Protocol::commandDef(cmdCode);
        m_logTab->logFrame(frame, true, QString("[0x%1] %2").arg(cmdCode, 2, 16, QChar('0')).toUpper().arg(cmd.name));
        m_realtimeTab->appendHexDisplay(frame, true);
    });

    // Command widget — manual send
    connect(m_commandTab, &CommandWidget::sendCommand, this, [this](quint8 cmdCode, const QByteArray& params) {
        QByteArray frame = m_protocol->buildRequest(cmdCode, params);
        m_waitingManualResponse = true;
        m_client->sendFrame(frame);

        auto cmd = Gsk988Protocol::commandDef(cmdCode);
        m_logTab->logFrame(frame, true, QString("[0x%1] %2").arg(cmdCode, 2, 16, QChar('0')).toUpper().arg(cmd.name));
    });

    // Mock server log
    connect(m_mockServer, &MockServer::logMessage, this, [this](const QString& msg) {
        m_logTab->logError(msg);
    });
}

void MainWindow::onConnectClicked()
{
    if (m_client->isConnected()) {
        // Disconnect
        m_realtimeTab->stopPolling();
        m_mockServer->stop();
        m_client->disconnect();
        return;
    }

    quint16 port = static_cast<quint16>(m_portSpin->value());
    bool isMock = (m_modeCombo->currentIndex() == 1);

    if (isMock) {
        if (!m_mockServer->start(port)) {
            m_logTab->logError("Mock服务器启动失败");
            return;
        }
        m_client->connectTo("127.0.0.1", port);
    } else {
        m_client->connectTo(m_ipEdit->text().trimmed(), port);
    }
}

void MainWindow::onResponseReceived(const QByteArray& frame)
{
    ParsedResponse resp = m_protocol->parseResponse(frame);

    auto cmd = Gsk988Protocol::commandDef(resp.cmdCode);
    QString desc = QString("[0x%1] %2").arg(resp.cmdCode, 2, 16, QChar('0')).toUpper().arg(cmd.name);

    if (!resp.isValid) {
        desc += " — " + resp.errorString;
    } else {
        desc += " — 成功";
    }

    m_logTab->logFrame(frame, false, desc);
    m_realtimeTab->appendHexDisplay(frame, false);

    // Route to appropriate tab FIRST (before starting polling)
    m_realtimeTab->updateData(resp);

    // Start polling after receiving the permission (0x0A) response
    if (m_needStartPolling) {
        m_needStartPolling = false;
        m_realtimeTab->startPolling();
    }

    // Only show in command tab if this was a manual send
    if (m_waitingManualResponse) {
        m_waitingManualResponse = false;
        QString interp = Gsk988Protocol::interpretData(resp.cmdCode, resp.rawData);
        m_commandTab->showResponse(resp, interp);
    }
}

void MainWindow::onResponseTimeout()
{
    m_logTab->logError("响应超时 (3秒)");

    // Advance polling state on timeout
    ParsedResponse dummy;
    dummy.cmdCode = 0;
    dummy.isValid = false;
    m_realtimeTab->updateData(dummy);
}

void MainWindow::updateConnectionIndicator(int state)
{
    QString color;
    switch (static_cast<ConnState>(state)) {
    case Connected:  color = "#4CAF50"; break;
    case Error:      color = "#F44336"; break;
    case Disconnected:
    default:         color = "gray"; break;
    }
    m_statusIndicator->setStyleSheet(
        QString("background: %1; border-radius: 7px;").arg(color));
}

void MainWindow::notifyXtuis(const QString& title, const QString& content)
{
    static const QString token = "GxtK0TGB4okvVfX7CxsxtBeDg";
    QUrl url(QString("https://wx.xtuis.cn/%1.send").arg(token));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("text", title);
    params.addQueryItem("desp", content);

    auto* manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, manager, &QNetworkAccessManager::deleteLater);
    manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
}
