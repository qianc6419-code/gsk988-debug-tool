#include "mainwindow.h"
#include "transport/tcptransport.h"
#include "transport/serialtransport.h"
#include "protocol/gsk988/gsk988protocol.h"
#include "protocol/gsk988/gsk988widgetfactory.h"
#include "protocol/gsk988/gsk988realtimewidget.h"
#include "protocol/gsk988/gsk988commandwidget.h"
#include "protocol/modbus/modbusprotocol.h"
#include "protocol/modbus/modbuswidgetfactory.h"
#include "protocol/modbus/modbusrealtimewidget.h"
#include "protocol/modbus/modbuscommandwidget.h"
#include "network/mockserver.h"
#include "ui/logwidget.h"
#include "protocol/iprotocol.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_logTab(new LogWidget)
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(3000);
    connect(m_timeoutTimer, &QTimer::timeout, this, &MainWindow::onResponseTimeout);

    setWindowTitle("多协议以太网调试工具");
    resize(1100, 700);
    setMinimumSize(900, 600);

    setupToolBar();
    setupTabs();

    // Create initial protocol and transport (both index 0)
    switchProtocol(0);
    switchTransport(0);

    // Wire toolbar signals
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProtocolChanged);
    connect(m_transportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTransportChanged);

    updateConnectionIndicator(Disconnected);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupToolBar()
{
    auto* toolbar = addToolBar("工具栏");
    toolbar->setMovable(false);

    // Protocol combo
    toolbar->addWidget(new QLabel(" 协议: "));
    m_protocolCombo = new QComboBox;
    m_protocolCombo->addItem("GSK988");
    m_protocolCombo->addItem("Modbus TCP");
    toolbar->addWidget(m_protocolCombo);

    toolbar->addSeparator();

    // Transport combo
    toolbar->addWidget(new QLabel(" 传输: "));
    m_transportCombo = new QComboBox;
    m_transportCombo->addItems({"TCP", "串口"});
    toolbar->addWidget(m_transportCombo);

    toolbar->addSeparator();

    // Dynamic transport config area — placeholder label
    m_transportConfigWidget = new QLabel(" ");
    toolbar->addWidget(m_transportConfigWidget);

    toolbar->addSeparator();

    // Mode combo
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

    // Log tab is always at the last position
    m_tabWidget->addTab(m_logTab, "通讯日志");
}

void MainWindow::switchProtocol(int index)
{
    Q_UNUSED(index);

    // If connected, disconnect first
    if (m_transport && m_transport->isConnected()) {
        m_transport->disconnectFromHost();
    }

    // Stop mock server if running
    if (m_mockServer) {
        m_mockServer->stop();
    }

    // Remove old protocol tabs (0..count-2, last tab is always LogWidget)
    while (m_tabWidget->count() > 1) {
        QWidget* w = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        delete w;
    }
    m_realtimeTab = nullptr;
    m_commandTab = nullptr;
    m_parseTab = nullptr;

    // Delete old protocol and factory
    delete m_protocol;
    m_protocol = nullptr;
    delete m_widgetFactory;
    m_widgetFactory = nullptr;

    // Create protocol and factory based on protocol combo index
    // Currently only GSK988 (index 0)
    switch (m_protocolCombo->currentIndex()) {
    case 1:
        m_protocol = new ModbusProtocol(this);
        m_widgetFactory = new ModbusWidgetFactory;
        break;
    default:
        m_protocol = new Gsk988Protocol(this);
        m_widgetFactory = new Gsk988WidgetFactory;
        break;
    }

    // Recreate mock server with the new protocol
    delete m_mockServer;
    m_mockServer = new MockServer(m_protocol, this);
    connect(m_mockServer, &MockServer::logMessage, this, [this](const QString& msg) {
        m_logTab->logError(msg);
    });

    // Create protocol-specific tabs via factory
    m_realtimeTab = m_widgetFactory->createRealtimeWidget();
    m_commandTab = m_widgetFactory->createCommandWidget();
    m_parseTab = m_widgetFactory->createParseWidget();

    // Insert before the log tab
    m_tabWidget->insertTab(0, m_realtimeTab, "实时数据");
    m_tabWidget->insertTab(1, m_commandTab, "发送指令");
    m_tabWidget->insertTab(2, m_parseTab, "数据解析");

    // Wire protocol-specific widget signals
    setupProtocolConnections();
}

void MainWindow::switchTransport(int index)
{
    Q_UNUSED(index);

    // If connected, disconnect first
    if (m_transport && m_transport->isConnected()) {
        m_transport->disconnectFromHost();
    }

    // Remove old transport config widget from toolbar
    if (m_transportConfigWidget) {
        m_transportConfigWidget->deleteLater();
        m_transportConfigWidget = nullptr;
    }

    // Delete old transport
    delete m_transport;
    m_transport = nullptr;

    // Create transport based on transport combo index
    int tidx = m_transportCombo ? m_transportCombo->currentIndex() : 0;
    switch (tidx) {
    case 1: m_transport = new SerialTransport(this); break;
    default: m_transport = new TcpTransport(this); break;
    }

    // Get config widget from transport and insert into toolbar
    m_transportConfigWidget = m_transport->createConfigWidget();

    // Connect transport signals
    connect(m_transport, &ITransport::connected, this, [this]() {
        updateConnectionIndicator(Connected);
        m_connectBtn->setText("断开");
        m_statusLabel->setText("已连接");

        // Send initial permission request (GSK988-specific: 0x0A)
        // Polling starts after receiving the 0x0A response
        m_buffer.clear();

        if (m_protocolCombo->currentIndex() == 0) {
            m_needStartPolling = true;
            QByteArray permFrame = m_protocol->buildRequest(0x0A);
            m_transport->send(permFrame);
            auto cmd = m_protocol->commandDef(0x0A);
            m_logTab->logFrame(permFrame, true, QString("[0x0A] %1").arg(cmd.name));
        } else {
            // Modbus TCP: start polling immediately
            m_needStartPolling = false;
            auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
            if (modbusRt) modbusRt->startPolling();
        }
    });

    connect(m_transport, &ITransport::disconnected, this, [this]() {
        updateConnectionIndicator(Disconnected);
        m_connectBtn->setText("连接");
        m_statusLabel->setText("未连接");
        m_timeoutTimer->stop();
        m_buffer.clear();

        // Stop polling on realtime widget
        auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
        if (rt) {
            rt->stopPolling();
            rt->clearData();
        }
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) {
            modbusRt->stopPolling();
            modbusRt->clearData();
        }
    });

    connect(m_transport, &ITransport::errorOccurred, this, [this](const QString& msg) {
        updateConnectionIndicator(Error);
        m_statusLabel->setText("连接失败");
        m_logTab->logError(msg);
    });

    connect(m_transport, &ITransport::dataReceived, this, &MainWindow::onDataReceived);
}

void MainWindow::setupProtocolConnections()
{
    // Wire GSK988-specific widget signals
    auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
    auto* cmd = qobject_cast<Gsk988CommandWidget*>(m_commandTab);

    if (rt) {
        connect(rt, &Gsk988RealtimeWidget::pollRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
            if (rt) rt->appendHexDisplay(frame, true);

            // Start timeout
            m_timeoutTimer->start();
        });
    }

    if (cmd) {
        connect(cmd, &Gsk988CommandWidget::sendCommand,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_waitingManualResponse = true;
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            // Start timeout
            m_timeoutTimer->start();
        });
    }

    // Modbus realtime widget
    auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
    if (modbusRt) {
        connect(modbusRt, &ModbusRealtimeWidget::pollRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* mrt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
            if (mrt) mrt->appendHexDisplay(frame, true);

            m_timeoutTimer->start();
        });
    }

    // Modbus command widget
    auto* modbusCmd = qobject_cast<ModbusCommandWidget*>(m_commandTab);
    if (modbusCmd) {
        connect(modbusCmd, &ModbusCommandWidget::sendCommand,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_waitingManualResponse = true;
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            m_timeoutTimer->start();
        });
    }
}

void MainWindow::onDataReceived(const QByteArray& rawData)
{
    m_buffer.append(rawData);

    // Extract complete frames from buffer using protocol
    forever {
        QByteArray frame = m_protocol->extractFrame(m_buffer);
        if (frame.isEmpty()) break;

        // Stop timeout timer — we got a response
        m_timeoutTimer->stop();

        // Parse the frame
        ParsedResponse resp = m_protocol->parseResponse(frame);

        auto cdef = m_protocol->commandDef(resp.cmdCode);
        QString desc = QString("[0x%1] %2")
                           .arg(resp.cmdCode, 2, 16, QChar('0')).toUpper()
                           .arg(cdef.name);
        if (resp.isValid) {
            desc += " — 成功";
        }

        m_logTab->logFrame(frame, false, desc);

        // Route to realtime tab
        auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
        if (rt) {
            rt->appendHexDisplay(frame, false);
            rt->updateData(resp);
        }

        // Route to Modbus realtime tab
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) {
            modbusRt->appendHexDisplay(frame, false);
            modbusRt->updateData(resp);
        }

        // Start polling after receiving the permission (0x0A) response
        if (m_needStartPolling) {
            m_needStartPolling = false;
            if (rt) rt->startPolling();
        }

        // Only show in command tab if this was a manual send
        if (m_waitingManualResponse) {
            m_waitingManualResponse = false;
            auto* cmdW = qobject_cast<Gsk988CommandWidget*>(m_commandTab);
            if (cmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                cmdW->showResponse(resp, interp);
            }
            auto* modbusCmdW = qobject_cast<ModbusCommandWidget*>(m_commandTab);
            if (modbusCmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                modbusCmdW->showResponse(resp, interp);
            }
        }
    }
}

void MainWindow::onResponseTimeout()
{
    m_logTab->logError("响应超时 (3秒)");

    // Advance polling state on timeout
    ParsedResponse dummy;
    dummy.cmdCode = 0;
    dummy.isValid = false;

    auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
    if (rt) rt->updateData(dummy);

    auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
    if (modbusRt) modbusRt->updateData(dummy);
}

void MainWindow::onConnectClicked()
{
    if (m_transport && m_transport->isConnected()) {
        // Disconnect
        auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
        if (rt) rt->stopPolling();
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) modbusRt->stopPolling();
        if (m_mockServer) m_mockServer->stop();
        m_transport->disconnectFromHost();
        return;
    }

    if (!m_transport) return;

    bool isMock = (m_modeCombo->currentIndex() == 1);

    if (isMock) {
        // For mock mode, we need the port from the transport config.
        // TcpTransport stores the port internally; we start mock server
        // on the same port, then connect transport to localhost.
        // Since TcpTransport manages its own IP+port via its config widget,
        // we need to extract the port. Use qobject_cast.
        auto* tcp = qobject_cast<TcpTransport*>(m_transport);
        quint16 port = 6000;
        if (tcp) {
            // Access the config widget values — TcpTransport reads from its
            // line edits when connectToHost() is called. For mock, we need
            // to know the port first. Read from the stored spin box.
            // We use a trick: get the config widget and find the spin box child.
            // But simpler: just use a fixed default or access via property.
            // Actually, the cleanest way: start mock on a fixed port, then
            // override the transport's target to 127.0.0.1:that_port.
            // For now, use a well-known approach — read port from config widget.
            QSpinBox* portSpin = m_transportConfigWidget->findChild<QSpinBox*>();
            if (portSpin) port = static_cast<quint16>(portSpin->value());
        }

        if (!m_mockServer->start(port)) {
            m_logTab->logError("Mock服务器启动失败");
            return;
        }

        // Override transport target to localhost for mock mode.
        // Set the IP and port in the transport's config widget.
        auto* tcp2 = qobject_cast<TcpTransport*>(m_transport);
        if (tcp2) {
            QLineEdit* ipEdit = m_transportConfigWidget->findChild<QLineEdit*>();
            QSpinBox* portSpin = m_transportConfigWidget->findChild<QSpinBox*>();
            if (ipEdit) ipEdit->setText("127.0.0.1");
            if (portSpin) portSpin->setValue(port);
        }

        m_transport->connectToHost();
    } else {
        m_transport->connectToHost();
    }
}

void MainWindow::onProtocolChanged()
{
    switchProtocol(m_protocolCombo->currentIndex());
}

void MainWindow::onTransportChanged()
{
    switchTransport(m_transportCombo->currentIndex());
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
