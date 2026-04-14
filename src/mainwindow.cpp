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
#include "protocol/fanuc/fanucprotocol.h"
#include "protocol/fanuc/fanucwidgetfactory.h"
#include "protocol/fanuc/fanucrealtimewidget.h"
#include "protocol/fanuc/fanuccommandwidget.h"
#include "protocol/siemens/siemensprotocol.h"
#include "protocol/siemens/siemenswidgetfactory.h"
#include "protocol/siemens/siemensrealtimewidget.h"
#include "protocol/siemens/siemenscommandwidget.h"
#include "protocol/mazak/mazakprotocol.h"
#include "protocol/mazak/mazakwidgetfactory.h"
#include "protocol/mazak/mazakrealtimewidget.h"
#include "protocol/mazak/mazakcommandwidget.h"
#include "protocol/mazak/mazakdllwrapper.h"
#include "protocol/knd/kndprotocol.h"
#include "protocol/knd/kndwidgetfactory.h"
#include "protocol/knd/kndrealtimewidget.h"
#include "protocol/knd/kndcommandwidget.h"
#include "protocol/knd/kndrestclient.h"
#include "protocol/syntec/syntecprotocol.h"
#include "protocol/syntec/syntecwidgetfactory.h"
#include "protocol/syntec/syntecrealtimewidget.h"
#include "protocol/syntec/synteccommandwidget.h"
#include "network/mockserver.h"
#include "ui/logwidget.h"
#include "protocol/iprotocol.h"
#include <QDebug>

#include <QToolBar>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QCoreApplication>
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

    // Wire toolbar signals BEFORE creating initial protocol/transport
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProtocolChanged);
    connect(m_transportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTransportChanged);

    // Create initial protocol and transport (both index 0)
    // Block signals during setup to avoid triggering switchProtocol/switchTransport twice
    m_protocolCombo->blockSignals(true);
    m_transportCombo->blockSignals(true);
    switchProtocol(0);
    switchTransport(0);
    m_protocolCombo->blockSignals(false);
    m_transportCombo->blockSignals(false);

    updateConnectionIndicator(Disconnected);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupToolBar()
{
    m_toolbar = addToolBar("工具栏");
    m_toolbar->setMovable(false);

    // Protocol combo
    m_toolbar->addWidget(new QLabel(" 协议: "));
    m_protocolCombo = new QComboBox;
    m_protocolCombo->addItem("GSK988");
    m_protocolCombo->addItem("Modbus TCP");
    m_protocolCombo->addItem("Fanuc FOCAS");
    m_protocolCombo->addItem("Siemens S7");
    m_protocolCombo->addItem("Mazak Smooth");
    m_protocolCombo->addItem("KND REST API");
    m_protocolCombo->addItem("Syntec 新代");
    m_toolbar->addWidget(m_protocolCombo);

    m_toolbar->addSeparator();

    // Transport combo
    m_toolbar->addWidget(new QLabel(" 传输: "));
    m_transportCombo = new QComboBox;
    m_transportCombo->addItems({"TCP", "串口"});
    m_toolbar->addWidget(m_transportCombo);

    m_toolbar->addSeparator();

    // Dynamic transport config area — placeholder label
    m_transportConfigWidget = new QLabel(" ");
    m_transportConfigAction = m_toolbar->addWidget(m_transportConfigWidget);

    m_toolbar->addSeparator();

    // Mode combo
    m_modeCombo = new QComboBox;
    m_modeCombo->addItems({"真实设备", "Mock模式"});
    m_toolbar->addWidget(m_modeCombo);

    m_toolbar->addSeparator();

    // Connect button
    m_connectBtn = new QPushButton("连接");
    m_connectBtn->setFixedWidth(70);
    m_toolbar->addWidget(m_connectBtn);

    // Status indicator
    m_statusIndicator = new QLabel;
    m_statusIndicator->setFixedSize(14, 14);
    m_statusIndicator->setStyleSheet("background: gray; border-radius: 7px;");
    m_toolbar->addWidget(m_statusIndicator);

    m_statusLabel = new QLabel("未连接");
    m_toolbar->addWidget(m_statusLabel);
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

    // Stop polling on current realtime widget before switching
    if (m_realtimeTab) {
        auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
        if (rt) rt->stopPolling();
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) modbusRt->stopPolling();
        auto* fanucRt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
        if (fanucRt) fanucRt->stopPolling();
        auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
        if (siemensRt) siemensRt->stopPolling();
        auto* mazakRt = qobject_cast<MazakRealtimeWidget*>(m_realtimeTab);
        if (mazakRt) mazakRt->stopPolling();
        auto* kndRt = qobject_cast<KndRealtimeWidget*>(m_realtimeTab);
        if (kndRt) kndRt->stopPolling();
        auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRt) syntecRt->stopPolling();
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
    case 2:
        m_protocol = new FanucProtocol(this);
        m_widgetFactory = new FanucWidgetFactory;
        break;
    case 3:
        m_protocol = new SiemensProtocol(this);
        m_widgetFactory = new SiemensWidgetFactory;
        break;
    case 4:
        m_protocol = new MazakProtocol(this);
        m_widgetFactory = new MazakWidgetFactory;
        break;
    case 5:
        m_protocol = new KndProtocol(this);
        m_widgetFactory = new KndWidgetFactory;
        break;
    case 6:
        m_protocol = new SyntecProtocol(this);
        m_widgetFactory = new SyntecWidgetFactory;
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

    // Default to realtime data page
    m_tabWidget->setCurrentIndex(0);

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

    // Remove old transport config widget from toolbar and remember position
    QAction* insertBefore = nullptr;
    if (m_transportConfigAction && m_toolbar) {
        QList<QAction*> actions = m_toolbar->actions();
        int idx = actions.indexOf(m_transportConfigAction);
        if (idx >= 0 && idx + 1 < actions.size()) {
            insertBefore = actions.at(idx + 1);
        }
        m_toolbar->removeAction(m_transportConfigAction);
        m_transportConfigAction = nullptr;
    }
    delete m_transportConfigWidget;
    m_transportConfigWidget = nullptr;

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
    if (m_toolbar) {
        if (insertBefore) {
            m_transportConfigAction = m_toolbar->insertWidget(insertBefore, m_transportConfigWidget);
        } else {
            m_transportConfigAction = m_toolbar->addWidget(m_transportConfigWidget);
        }
    }

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
        } else if (m_protocolCombo->currentIndex() == 1) {
            // Modbus TCP: start polling immediately
            m_needStartPolling = false;
            auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
            if (modbusRt) modbusRt->startPolling();
        } else if (m_protocolCombo->currentIndex() == 2) {
            // Fanuc FOCAS: send handshake first, polling starts after handshake response
            m_needStartPolling = true;
            QByteArray handshakeFrame = qobject_cast<FanucProtocol*>(m_protocol)->buildHandshake();
            m_transport->send(handshakeFrame);
            m_logTab->logFrame(handshakeFrame, true, "[握手] Fanuc FOCAS");
        } else if (m_protocolCombo->currentIndex() == 3) {
            // Siemens S7: 3-step handshake
            m_needStartPolling = true;
            auto* siemens = qobject_cast<SiemensProtocol*>(m_protocol);
            QByteArray frame = siemens->buildHandshake();
            m_transport->send(frame);
            m_logTab->logFrame(frame, true, "[握手1/3] Siemens S7");
        } else if (m_protocolCombo->currentIndex() == 6) {
            // Syntec: send handshake, polling starts after response
            m_needStartPolling = true;
            auto* syntec = qobject_cast<SyntecProtocol*>(m_protocol);
            QByteArray handshakeFrame = syntec->buildHandshake();
            m_transport->send(handshakeFrame);
            m_logTab->logFrame(handshakeFrame, true, "[握手] Syntec 新代");
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
        auto* fanucRt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
        if (fanucRt) {
            fanucRt->stopPolling();
            fanucRt->clearData();
        }
        auto* siemensRtDisc = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
        if (siemensRtDisc) {
            siemensRtDisc->stopPolling();
            siemensRtDisc->clearData();
        }
        auto* mazakRtDisc = qobject_cast<MazakRealtimeWidget*>(m_realtimeTab);
        if (mazakRtDisc) {
            mazakRtDisc->stopPolling();
            mazakRtDisc->clearData();
        }
        auto* kndRtDisc2 = qobject_cast<KndRealtimeWidget*>(m_realtimeTab);
        if (kndRtDisc2) {
            kndRtDisc2->stopPolling();
            kndRtDisc2->clearData();
        }
        auto* syntecRtDisc = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRtDisc) {
            syntecRtDisc->stopPolling();
            syntecRtDisc->clearData();
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

        connect(modbusRt, &ModbusRealtimeWidget::singleReadRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_waitingManualResponse = true;
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2 [单次读取]")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* mrt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
            if (mrt) mrt->appendHexDisplay(frame, true);

            m_timeoutTimer->start();
        });

        connect(modbusRt, &ModbusRealtimeWidget::timeoutChanged,
                this, [this](int ms) {
            m_timeoutTimer->setInterval(ms);
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

    // Fanuc realtime widget
    auto* fanucRt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
    if (fanucRt) {
        connect(fanucRt, &FanucRealtimeWidget::pollRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* mrt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
            if (mrt) mrt->appendHexDisplay(frame, true);

            m_timeoutTimer->start();
        });
    }

    // Fanuc command widget
    auto* fanucCmd = qobject_cast<FanucCommandWidget*>(m_commandTab);
    if (fanucCmd) {
        connect(fanucCmd, &FanucCommandWidget::sendCommand,
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

    // Siemens realtime widget
    auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
    if (siemensRt) {
        connect(siemensRt, &SiemensRealtimeWidget::pollRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* srt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
            if (srt) srt->appendHexDisplay(frame, true);

            m_timeoutTimer->start();
        });
    }

    // Siemens command widget
    auto* siemensCmd = qobject_cast<SiemensCommandWidget*>(m_commandTab);
    if (siemensCmd) {
        connect(siemensCmd, &SiemensCommandWidget::sendCommand,
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

    // Syntec realtime widget
    auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
    if (syntecRt) {
        connect(syntecRt, &SyntecRealtimeWidget::pollRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* srt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
            if (srt) srt->appendHexDisplay(frame, true);

            m_timeoutTimer->start();
        });
    }

    // Syntec command widget
    auto* syntecCmd = qobject_cast<SyntecCommandWidget*>(m_commandTab);
    if (syntecCmd) {
        connect(syntecCmd, &SyntecCommandWidget::sendCommand,
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

        // Route to Fanuc realtime tab
        auto* fanucRt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
        if (fanucRt) {
            fanucRt->appendHexDisplay(frame, false);
            fanucRt->updateData(resp);
        }

        // Handle Siemens S7 multi-step handshake
        auto* siemens = qobject_cast<SiemensProtocol*>(m_protocol);
        if (siemens && !siemens->isHandshakeComplete()) {
            int step = siemens->handshakeStep();
            QByteArray nextFrame = siemens->advanceHandshake();
            if (!nextFrame.isEmpty()) {
                m_transport->send(nextFrame);
                m_logTab->logFrame(nextFrame, true,
                    QString("[握手%1/3] Siemens S7").arg(step + 1));
            }
            if (siemens->isHandshakeComplete() && m_needStartPolling) {
                m_needStartPolling = false;
                auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
                if (siemensRt) siemensRt->startPolling();
            }
            continue;  // Don't route handshake frames to realtime tab
        }

        // Route to Siemens realtime tab
        auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
        if (siemensRt) {
            siemensRt->appendHexDisplay(frame, false);
            siemensRt->updateData(resp);
        }

        // Route to Syntec realtime tab
        auto* syntecRtData = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRtData) {
            syntecRtData->appendHexDisplay(frame, false);
            syntecRtData->updateData(resp);
        }

        // Start polling after receiving the permission (0x0A) response
        if (m_needStartPolling) {
            m_needStartPolling = false;
            if (rt) rt->startPolling();
            if (fanucRt) fanucRt->startPolling();
            if (siemensRt) siemensRt->startPolling();
            if (syntecRtData) syntecRtData->startPolling();
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
            // Route single read result to Modbus realtime widget
            auto* modbusRtForSingle = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
            if (modbusRtForSingle) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                modbusRtForSingle->showSingleReadResult(resp, interp);
            }
            auto* fanucCmdW = qobject_cast<FanucCommandWidget*>(m_commandTab);
            if (fanucCmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                fanucCmdW->showResponse(resp, interp);
            }
            auto* siemensCmdW = qobject_cast<SiemensCommandWidget*>(m_commandTab);
            if (siemensCmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                siemensCmdW->showResponse(resp, interp);
            }
            auto* syntecCmdW = qobject_cast<SyntecCommandWidget*>(m_commandTab);
            if (syntecCmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                syntecCmdW->showResponse(resp, interp);
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

    auto* fanucRt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
    if (fanucRt) fanucRt->updateData(dummy);

    auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
    if (siemensRt) siemensRt->updateData(dummy);

    auto* syntecRtTimeout = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
    if (syntecRtTimeout) syntecRtTimeout->updateData(dummy);
}

void MainWindow::onConnectClicked()
{
    // === Mazak 协议使用 DLL 连接，不走 Transport 层 ===
    if (m_protocolCombo->currentIndex() == 4) {
        auto& dll = MazakDLLWrapper::instance();
        if (dll.isConnected()) {
            // 断开
            auto* mazakRt = qobject_cast<MazakRealtimeWidget*>(m_realtimeTab);
            if (mazakRt) { mazakRt->stopPolling(); mazakRt->clearData(); }
            dll.disconnect(dll.handle());
            dll.clearConnection();
            updateConnectionIndicator(Disconnected);
            m_connectBtn->setText("连接");
            m_statusLabel->setText("未连接");
            return;
        }

        bool isMock = (m_modeCombo->currentIndex() == 1);

        // Mock 模式：不加载 DLL，直接模拟连接
        if (isMock) {
            dll.setHandle(1); // 模拟句柄
            updateConnectionIndicator(Connected);
            m_connectBtn->setText("断开");
            m_statusLabel->setText("已连接(Mock)");
            auto* mazakRt = qobject_cast<MazakRealtimeWidget*>(m_realtimeTab);
            if (mazakRt) mazakRt->startPolling();
            return;
        }

        // 真实模式：加载 DLL 并连接
        QLineEdit* ipEdit = m_transportConfigWidget ? m_transportConfigWidget->findChild<QLineEdit*>() : nullptr;
        QSpinBox* portSpin = m_transportConfigWidget ? m_transportConfigWidget->findChild<QSpinBox*>() : nullptr;
        QString ip = ipEdit ? ipEdit->text() : "192.168.1.1";
        int port = portSpin ? portSpin->value() : 10000;

        if (!dll.isLoaded()) {
            QString dllPath = QCoreApplication::applicationDirPath() + "/NTIFDLL";
            if (!dll.load(dllPath)) {
                m_logTab->logError("无法加载 NTIFDLL.dll: " + dllPath + ".dll (DLL可能需要32位编译)");
                updateConnectionIndicator(Error);
                m_statusLabel->setText("DLL加载失败");
                return;
            }
        }

        unsigned short handle = 0;
        if (dll.connect(handle, ip, port, 3000)) {
            dll.setHandle(handle);
            updateConnectionIndicator(Connected);
            m_connectBtn->setText("断开");
            m_statusLabel->setText("已连接");
            auto* mazakRt = qobject_cast<MazakRealtimeWidget*>(m_realtimeTab);
            if (mazakRt) mazakRt->startPolling();
        } else {
            updateConnectionIndicator(Error);
            m_statusLabel->setText("连接失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
            m_logTab->logError("Mazak 连接失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
        return;
    }

    // === KND 协议使用 HTTP REST API，不走 Transport 层 ===
    if (m_protocolCombo->currentIndex() == 5) {
        auto& client = KndRestClient::instance();
        if (client.isConnected()) {
            auto* kndRt = qobject_cast<KndRealtimeWidget*>(m_realtimeTab);
            if (kndRt) { kndRt->stopPolling(); kndRt->clearData(); }
            client.setConnected(false);
            updateConnectionIndicator(Disconnected);
            m_connectBtn->setText("连接");
            m_statusLabel->setText("未连接");
            return;
        }

        bool isMock = (m_modeCombo->currentIndex() == 1);

        QLineEdit* ipEdit = m_transportConfigWidget ? m_transportConfigWidget->findChild<QLineEdit*>() : nullptr;
        QString ip = ipEdit ? ipEdit->text() : "192.168.1.1";

        client.setBaseUrl(ip);
        client.setMockMode(isMock);

        if (isMock) {
            client.setConnected(true);
            updateConnectionIndicator(Connected);
            m_connectBtn->setText("断开");
            m_statusLabel->setText("已连接(Mock)");
            auto* kndRt = qobject_cast<KndRealtimeWidget*>(m_realtimeTab);
            if (kndRt) kndRt->startPolling();
            return;
        }

        // 真实模式：发送测试请求验证连接
        m_logTab->logError("正在连接 KND REST API: " + ip + "...");
        connect(&client, &KndRestClient::statusReceived, this, [this](const QJsonObject&) {
            auto& c = KndRestClient::instance();
            c.setConnected(true);
            updateConnectionIndicator(Connected);
            m_connectBtn->setText("断开");
            m_statusLabel->setText("已连接");
            auto* kndRt = qobject_cast<KndRealtimeWidget*>(m_realtimeTab);
            if (kndRt) kndRt->startPolling();
            disconnect(&c, &KndRestClient::statusReceived, this, 0);
        }, Qt::UniqueConnection);

        connect(&client, &KndRestClient::errorOccurred, this, [this](const QString& msg) {
            static bool firstError = true;
            if (!firstError) return;
            firstError = false;
            auto& c = KndRestClient::instance();
            c.setConnected(false);
            updateConnectionIndicator(Error);
            m_statusLabel->setText("连接失败");
            m_logTab->logError("KND 连接失败: " + msg);
        }, Qt::UniqueConnection);

        client.getStatus();
        return;
    }

    // === 其他协议使用 Transport 层 ===
    if (m_transport && m_transport->isConnected()) {
        // Disconnect
        auto* rt = qobject_cast<Gsk988RealtimeWidget*>(m_realtimeTab);
        if (rt) rt->stopPolling();
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) modbusRt->stopPolling();
        auto* fanucRt = qobject_cast<FanucRealtimeWidget*>(m_realtimeTab);
        if (fanucRt) fanucRt->stopPolling();
        auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
        if (siemensRt) siemensRt->stopPolling();
        auto* mazakRt = qobject_cast<MazakRealtimeWidget*>(m_realtimeTab);
        if (mazakRt) mazakRt->stopPolling();
        auto* kndRtDisc = qobject_cast<KndRealtimeWidget*>(m_realtimeTab);
        if (kndRtDisc) kndRtDisc->stopPolling();
        auto* syntecRtConn = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRtConn) syntecRtConn->stopPolling();
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
