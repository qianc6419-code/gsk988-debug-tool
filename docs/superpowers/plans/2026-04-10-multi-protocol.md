# Multi-Protocol Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor GSK988 debug tool into a multi-protocol architecture supporting pluggable protocols and transports.

**Architecture:** Plugin-style with three layers — Transport (TCP/serial raw bytes), Protocol (frame format + command definitions), UI (protocol-specific widgets). MainWindow acts as orchestrator, switching between protocols at runtime.

**Tech Stack:** Qt 5.15.2 (Widgets + Network + SerialPort), C++14, MinGW + MSVC

**Spec:** `docs/superpowers/specs/2026-04-10-multi-protocol-design.md`

---

## File Structure (Final State)

```
src/
  transport/
    itransport.h              # Abstract transport interface
    tcptransport.h/.cpp       # TCP implementation
    serialtransport.h/.cpp    # Serial port implementation (stub)
  protocol/
    iprotocol.h               # Abstract protocol interface + shared types
    gsk988/
      gsk988protocol.h/.cpp   # Implements IProtocol
      gsk988framebuilder.h/.cpp  # GSK988-specific frame building
      gsk988widgetfactory.h/.cpp # Creates GSK988 UI tabs
      gsk988realtimewidget.h/.cpp
      gsk988commandwidget.h/.cpp
      gsk988parsewidget.h/.cpp
    modbus/                   # (Future iteration)
  ui/
    iprotocolwidgetfactory.h  # Abstract factory interface
    logwidget.h/.cpp          # Shared across protocols
  mainwindow.h/.cpp           # Orchestrator
  main.cpp

Files to DELETE after migration:
  src/network/tcpclient.h/.cpp
  src/network/mockserver.h/.cpp
  src/protocol/framebuilder.h/.cpp
  src/protocol/gsk988protocol.h/.cpp
  src/ui/realtimewidget.h/.cpp
  src/ui/commandwidget.h/.cpp
  src/ui/parsewidget.h/.cpp
```

---

### Task 1: Define Interfaces

**Files:**
- Create: `src/transport/itransport.h`
- Create: `src/protocol/iprotocol.h`
- Create: `src/ui/iprotocolwidgetfactory.h`

- [ ] **Step 1: Create `src/transport/itransport.h`**

```cpp
#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include <QObject>
#include <QByteArray>

class QWidget;

class ITransport : public QObject {
    Q_OBJECT
public:
    virtual ~ITransport() = default;
    virtual QString name() const = 0;
    virtual QWidget* createConfigWidget() = 0;
    virtual void connectToHost() = 0;
    virtual void disconnectFromHost() = 0;
    virtual bool isConnected() const = 0;
    virtual void send(const QByteArray& data) = 0;

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& msg);
};

#endif // ITRANSPORT_H
```

- [ ] **Step 2: Create `src/protocol/iprotocol.h`**

```cpp
#ifndef IPROTOCOL_H
#define IPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVector>

struct ProtocolCommand {
    quint8 cmdCode;
    QString name;
    QString category;
    QString paramDesc;
    QString respDesc;
};

struct ParsedResponse {
    quint8 cmdCode = 0;
    bool isValid = false;
    QByteArray rawData;
};

class IProtocol : public QObject {
    Q_OBJECT
public:
    virtual ~IProtocol() = default;
    virtual QString name() const = 0;
    virtual QString version() const = 0;

    virtual QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) = 0;
    virtual ParsedResponse parseResponse(const QByteArray& frame) = 0;
    virtual QByteArray extractFrame(QByteArray& buffer) = 0;

    virtual QVector<ProtocolCommand> allCommands() const = 0;
    virtual ProtocolCommand commandDef(quint8 cmdCode) const = 0;

    virtual QString interpretData(quint8 cmdCode, const QByteArray& data) const = 0;

    virtual QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const = 0;
};

#endif // IPROTOCOL_H
```

- [ ] **Step 3: Create `src/ui/iprotocolwidgetfactory.h`**

```cpp
ifndef IPROTOCOLWIDGETFACTORY_H
#define IPROTOCOLWIDGETFACTORY_H

class QWidget;

class IProtocolWidgetFactory {
public:
    virtual ~IProtocolWidgetFactory() = default;
    virtual QWidget* createRealtimeWidget() = 0;
    virtual QWidget* createCommandWidget() = 0;
    virtual QWidget* createParseWidget() = 0;
};

#endif // IPROTOCOLWIDGETFACTORY_H
```

- [ ] **Step 4: Build and verify interfaces compile**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`
Note: New headers won't affect build yet since nothing includes them. Just verify no existing code breaks.

- [ ] **Step 5: Commit**

```bash
git add src/transport/itransport.h src/protocol/iprotocol.h src/ui/iprotocolwidgetfactory.h
git commit -m "feat: define ITransport, IProtocol, IProtocolWidgetFactory interfaces"
```

---

### Task 2: Implement TcpTransport

**Files:**
- Create: `src/transport/tcptransport.h`
- Create: `src/transport/tcptransport.cpp`

This replaces `TcpClient`. Key difference: no frame parsing — just sends/receives raw bytes.

- [ ] **Step 1: Create `src/transport/tcptransport.h`**

```cpp
#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "itransport.h"
#include <QTcpSocket>
#include <QLineEdit>
#include <QSpinBox>

class TcpTransport : public ITransport {
    Q_OBJECT
public:
    explicit TcpTransport(QObject* parent = nullptr);
    ~TcpTransport();

    QString name() const override { return "TCP"; }
    QWidget* createConfigWidget() override;
    void connectToHost() override;
    void disconnectFromHost() override;
    bool isConnected() const override;
    void send(const QByteArray& data) override;

private slots:
    void onReadyRead();

private:
    QTcpSocket* m_socket;
    QLineEdit* m_ipEdit;
    QSpinBox* m_portSpin;
};

#endif // TCPTRANSPORT_H
```

- [ ] **Step 2: Create `src/transport/tcptransport.cpp`**

```cpp
#include "tcptransport.h"
#include <QHBoxLayout>
#include <QLabel>

TcpTransport::TcpTransport(QObject* parent)
    : ITransport(parent)
    , m_socket(new QTcpSocket(this))
    , m_ipEdit(nullptr)
    , m_portSpin(nullptr)
{
    connect(m_socket, &QTcpSocket::connected, this, &TcpTransport::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        emit disconnected();
    });

    using ErrorSignal = void (QTcpSocket::*)(QAbstractSocket::SocketError);
    connect(m_socket, static_cast<ErrorSignal>(&QTcpSocket::errorOccurred), this,
            [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket->errorString());
    });

    connect(m_socket, &QTcpSocket::readyRead, this, &TcpTransport::onReadyRead);
}

TcpTransport::~TcpTransport()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();
}

QWidget* TcpTransport::createConfigWidget()
{
    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QLabel("IP:"));
    m_ipEdit = new QLineEdit("192.168.1.100");
    m_ipEdit->setFixedWidth(130);
    layout->addWidget(m_ipEdit);

    layout->addWidget(new QLabel("端口:"));
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(6000);
    m_portSpin->setFixedWidth(80);
    layout->addWidget(m_portSpin);

    return widget;
}

void TcpTransport::connectToHost()
{
    if (!m_ipEdit || !m_portSpin) return;
    m_socket->connectToHost(m_ipEdit->text().trimmed(),
                            static_cast<quint16>(m_portSpin->value()));
}

void TcpTransport::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

bool TcpTransport::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpTransport::send(const QByteArray& data)
{
    if (!isConnected()) return;
    m_socket->write(data);
}

void TcpTransport::onReadyRead()
{
    emit dataReceived(m_socket->readAll());
}
```

- [ ] **Step 3: Build and verify**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`

- [ ] **Step 4: Commit**

```bash
git add src/transport/tcptransport.h src/transport/tcptransport.cpp
git commit -m "feat: add TcpTransport implementing ITransport"
```

---

### Task 3: Move GSK988 Protocol to Plugin Structure

**Files:**
- Create: `src/protocol/gsk988/gsk988framebuilder.h` (copy from `src/protocol/framebuilder.h`)
- Create: `src/protocol/gsk988/gsk988framebuilder.cpp` (copy from `src/protocol/framebuilder.cpp`)
- Create: `src/protocol/gsk988/gsk988protocol.h` (adapt from `src/protocol/gsk988protocol.h`)
- Create: `src/protocol/gsk988/gsk988protocol.cpp` (adapt from `src/protocol/gsk988protocol.cpp`)

This task copies existing code into the new directory structure and adapts it to implement `IProtocol`. Old files are NOT deleted yet — we keep them until MainWindow is refactored.

- [ ] **Step 1: Create directory and copy framebuilder**

Create `src/protocol/gsk988/` directory.

Copy `src/protocol/framebuilder.h` to `src/protocol/gsk988/gsk988framebuilder.h`, rename class from `FrameBuilder` to `Gsk988FrameBuilder`, update include guard.

Copy `src/protocol/framebuilder.cpp` to `src/protocol/gsk988/gsk988framebuilder.cpp`, update include and class name.

The `extractFrame` method stays on `Gsk988FrameBuilder` for now — will be exposed via `Gsk988Protocol::extractFrame` in the next step.

- [ ] **Step 2: Create `src/protocol/gsk988/gsk988protocol.h`**

Adapted from current `gsk988protocol.h`. Implements `IProtocol`. Keeps `CommandDef` as internal type, maps to `ProtocolCommand`. Keeps `mockResponseData` as static method, exposes via `mockResponse`.

```cpp
#ifndef GSK988PROTOCOL_V2_H
#define GSK988PROTOCOL_V2_H

#include "protocol/iprotocol.h"

class Gsk988FrameBuilder;

class Gsk988Protocol : public IProtocol {
    Q_OBJECT
public:
    explicit Gsk988Protocol(QObject* parent = nullptr);

    QString name() const override { return "GSK988"; }
    QString version() const override { return "2.0"; }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;

    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    // Legacy static methods kept for compatibility during migration
    static QVector<struct CommandDefRaw> allCommandsRaw();
    static struct CommandDefRaw commandDefRaw(quint8 cmdCode);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);
};

// Internal command definition (keeps existing field names)
struct CommandDefRaw {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

#endif // GSK988PROTOCOL_V2_H
```

- [ ] **Step 3: Create `src/protocol/gsk988/gsk988protocol.cpp`**

Copy the body from current `gsk988protocol.cpp`. Key changes:
- Include `gsk988framebuilder.h` instead of `framebuilder.h`
- `buildRequest`: delegates to `Gsk988FrameBuilder::buildRequestFrame`
- `parseResponse`: same logic, maps internal error codes to `ParsedResponse` without `errorString` field
- `extractFrame`: delegates to `Gsk988FrameBuilder::extractFrame`
- `allCommands`: maps `CommandDefRaw` to `ProtocolCommand`
- `mockResponse`: delegates to static `mockResponseData`

- [ ] **Step 4: Build and verify both old and new code coexist**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`

- [ ] **Step 5: Commit**

```bash
git add src/protocol/gsk988/
git commit -m "feat: add GSK988 protocol plugin implementing IProtocol"
```

---

### Task 4: Move GSK988 UI Widgets + Create WidgetFactory

**Files:**
- Move: `src/ui/realtimewidget.h/.cpp` → `src/protocol/gsk988/gsk988realtimewidget.h/.cpp`
- Move: `src/ui/commandwidget.h/.cpp` → `src/protocol/gsk988/gsk988commandwidget.h/.cpp`
- Move: `src/ui/parsewidget.h/.cpp` → `src/protocol/gsk988/gsk988parsewidget.h/.cpp`
- Create: `src/protocol/gsk988/gsk988widgetfactory.h/.cpp`

Strategy: Copy files first (old files still exist), rename classes, update includes.

- [ ] **Step 1: Copy and rename RealtimeWidget**

Copy `src/ui/realtimewidget.h` → `src/protocol/gsk988/gsk988realtimewidget.h`
Copy `src/ui/realtimewidget.cpp` → `src/protocol/gsk988/gsk988realtimewidget.cpp`

Changes:
- Class name: `RealtimeWidget` → `Gsk988RealtimeWidget`
- Include guard: `REALTIMEWIDGET_H` → `GSK988REALTIMEWIDGET_H`
- Includes: `"protocol/gsk988protocol.h"` → `"gsk988protocol.h"` (same directory)
- All `ParsedResponse` references now use the one from `iprotocol.h`
- `CommandDef` references → `CommandDefRaw` (legacy compatibility)
- `Gsk988Protocol::allCommands()` → `Gsk988Protocol::allCommandsRaw()`
- `Gsk988Protocol::commandDef()` → `Gsk988Protocol::commandDefRaw()`

- [ ] **Step 2: Copy and rename CommandWidget**

Copy `src/ui/commandwidget.h` → `src/protocol/gsk988/gsk988commandwidget.h`
Copy `src/ui/commandwidget.cpp` → `src/protocol/gsk988/gsk988commandwidget.cpp`

Changes:
- Class name: `CommandWidget` → `Gsk988CommandWidget`
- Same include and type updates as RealtimeWidget
- `showResponse` signature uses `ParsedResponse` from `iprotocol.h`

- [ ] **Step 3: Copy and rename ParseWidget**

Copy `src/ui/parsewidget.h` → `src/protocol/gsk988/gsk988parsewidget.h`
Copy `src/ui/parsewidget.cpp` → `src/protocol/gsk988/gsk988parsewidget.cpp`

Changes:
- Class name: `ParseWidget` → `Gsk988ParseWidget`
- `FrameBuilder::` → `Gsk988FrameBuilder::`
- `Gsk988Protocol::` static calls updated for new class

- [ ] **Step 4: Create `src/protocol/gsk988/gsk988widgetfactory.h`**

```cpp
#ifndef GSK988WIDGETFACTORY_H
#define GSK988WIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class Gsk988WidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // GSK988WIDGETFACTORY_H
```

- [ ] **Step 5: Create `src/protocol/gsk988/gsk988widgetfactory.cpp`**

```cpp
#include "gsk988widgetfactory.h"
#include "gsk988realtimewidget.h"
#include "gsk988commandwidget.h"
#include "gsk988parsewidget.h"

QWidget* Gsk988WidgetFactory::createRealtimeWidget()
{
    return new Gsk988RealtimeWidget;
}

QWidget* Gsk988WidgetFactory::createCommandWidget()
{
    return new Gsk988CommandWidget;
}

QWidget* Gsk988WidgetFactory::createParseWidget()
{
    return new Gsk988ParseWidget;
}
```

- [ ] **Step 6: Build and verify**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`

- [ ] **Step 7: Commit**

```bash
git add src/protocol/gsk988/gsk988widgetfactory.h src/protocol/gsk988/gsk988widgetfactory.cpp
git add src/protocol/gsk988/gsk988realtimewidget.h src/protocol/gsk988/gsk988realtimewidget.cpp
git add src/protocol/gsk988/gsk988commandwidget.h src/protocol/gsk988/gsk988commandwidget.cpp
git add src/protocol/gsk988/gsk988parsewidget.h src/protocol/gsk988/gsk988parsewidget.cpp
git commit -m "feat: add GSK988 widget factory and moved UI widgets"
```

---

### Task 5: Refactor MockServer to be Generic

**Files:**
- Create: `src/network/mockserver.h` (overwrite existing, make generic)
- Create: `src/network/mockserver.cpp` (overwrite existing)

Change: MockServer takes an `IProtocol*` instead of hardcoding `Gsk988Protocol`. Uses `IProtocol::extractFrame` and `IProtocol::mockResponse`.

- [ ] **Step 1: Rewrite `src/network/mockserver.h`**

```cpp
#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class IProtocol;

class MockServer : public QObject {
    Q_OBJECT
public:
    explicit MockServer(IProtocol* protocol, QObject* parent = nullptr);
    ~MockServer();

    bool start(quint16 port = 6000);
    void stop();
    bool isRunning() const;
    quint16 serverPort() const;

signals:
    void logMessage(const QString& msg);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
    IProtocol* m_protocol;
};

#endif // MOCKSERVER_H
```

- [ ] **Step 2: Rewrite `src/network/mockserver.cpp`**

```cpp
#include "mockserver.h"
#include "protocol/iprotocol.h"

MockServer::MockServer(IProtocol* protocol, QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_protocol(protocol)
{
    connect(m_server, &QTcpServer::newConnection, this, &MockServer::onNewConnection);
}

MockServer::~MockServer()
{
    stop();
}

bool MockServer::start(quint16 port)
{
    if (m_server->isListening()) return true;
    bool ok = m_server->listen(QHostAddress::LocalHost, port);
    if (ok) {
        emit logMessage(QString("Mock服务器已启动，端口: %1").arg(port));
    } else {
        emit logMessage(QString("Mock服务器启动失败: %1").arg(m_server->errorString()));
    }
    return ok;
}

void MockServer::stop()
{
    for (auto* client : m_clients) {
        client->abort();
        client->deleteLater();
    }
    m_clients.clear();
    m_server->close();
}

bool MockServer::isRunning() const { return m_server->isListening(); }
quint16 MockServer::serverPort() const { return m_server->serverPort(); }

void MockServer::onNewConnection()
{
    QTcpSocket* client = m_server->nextPendingConnection();
    m_clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &MockServer::onClientReadyRead);
    connect(client, &QTcpSocket::disconnected, this, [this, client]() {
        m_clients.removeOne(client);
        client->deleteLater();
    });
    emit logMessage(QString("Mock客户端已连接: %1:%2")
                        .arg(client->peerAddress().toString())
                        .arg(client->peerPort()));
}

void MockServer::onClientReadyRead()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray buffer = client->readAll();

    QByteArray frame;
    while (!(frame = m_protocol->extractFrame(buffer)).isEmpty()) {
        // Parse request to get cmdCode — protocol-specific but we use
        // a generic approach: build a ParsedResponse from the frame
        ParsedResponse resp = m_protocol->parseResponse(frame);

        QByteArray respData = m_protocol->mockResponse(resp.cmdCode, resp.rawData);
        // For mock, we build a response frame by treating respData as a raw response
        // The protocol's buildRequest is for requests; we need a raw write.
        // Since mockResponse returns the full data field, we use buildRequest
        // as a wrapper — but this is protocol-specific.
        // Simpler approach: just write respData directly as a complete frame
        // by asking the protocol to build it.
        // For now, we send respData directly — the protocol implementation
        // handles framing internally in mockResponse.
        // Actually, mockResponse should return a complete frame.
        // Let's redefine: mockResponse returns a complete frame ready to send.
        client->write(respData);
        client->flush();
    }
}
```

Note: `IProtocol::mockResponse` should return a **complete frame** (ready to write to socket), not just the data field. This changes the interface slightly from the current `Gsk988Protocol::mockResponseData` which returns only the data field.

Update `src/protocol/iprotocol.h` doc comment:
```cpp
// Returns a complete response frame ready to send over the transport
virtual QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const = 0;
```

The GSK988 implementation wraps the data with `Gsk988FrameBuilder::buildResponseFrame`.

- [ ] **Step 3: Build and verify**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`
Expected: May fail because MainWindow still uses old MockServer constructor. This is OK — we fix it in Task 6.

- [ ] **Step 4: Commit**

```bash
git add src/network/mockserver.h src/network/mockserver.cpp
git commit -m "refactor: make MockServer generic, takes IProtocol pointer"
```

---

### Task 6: Refactor MainWindow as Orchestrator

**Files:**
- Modify: `src/mainwindow.h` (major rewrite)
- Modify: `src/mainwindow.cpp` (major rewrite)
- Modify: `GSK988Tool.pro` (add new source files, add serialport module)

This is the biggest task. MainWindow becomes the protocol/transport switcher.

- [ ] **Step 1: Rewrite `src/mainwindow.h`**

```cpp
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>
#include <QByteArray>

class ITransport;
class IProtocol;
class IProtocolWidgetFactory;
class MockServer;
class LogWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onProtocolChanged();
    void onTransportChanged();
    void onResponseReceived(const QByteArray& rawData);
    void onResponseTimeout();

private:
    void setupToolBar();
    void setupTabs();
    void setupConnections();
    void switchProtocol(int index);
    void switchTransport(int index);
    void updateConnectionIndicator(int state);
    void notifyXtuis(const QString& title, const QString& content);

    enum ConnState { Disconnected = 0, Connected = 1, Error = 2 };

    // Protocol/Transport management
    ITransport* m_transport = nullptr;
    IProtocol* m_protocol = nullptr;
    IProtocolWidgetFactory* m_widgetFactory = nullptr;
    MockServer* m_mockServer = nullptr;

    // Transport config widget (owned by toolbar layout)
    QWidget* m_transportConfigWidget = nullptr;

    // Toolbar controls
    QComboBox* m_protocolCombo;
    QComboBox* m_transportCombo;
    QComboBox* m_modeCombo;
    QPushButton* m_connectBtn;
    QLabel* m_statusIndicator;
    QLabel* m_statusLabel;

    // Tabs
    QTabWidget* m_tabWidget;
    QWidget* m_realtimeTab = nullptr;
    QWidget* m_commandTab = nullptr;
    QWidget* m_parseTab = nullptr;
    LogWidget* m_logTab;

    // State
    QTimer* m_timeoutTimer;
    QByteArray m_buffer;
    QByteArray m_pendingFrame;
    bool m_waitingManualResponse;
    bool m_needStartPolling;
};

#endif // MAINWINDOW_H
```

- [ ] **Step 2: Rewrite `src/mainwindow.cpp`**

Key responsibilities:
1. **Protocol/Transport combos**: Populated in constructor. Protocol switch creates new `IProtocol` + `IProtocolWidgetFactory` + replaces tabs. Transport switch creates new `ITransport` + replaces config widget.
2. **Data flow**: Transport emits `dataReceived` → MainWindow appends to `m_buffer` → `IProtocol::extractFrame` to get complete frames → `IProtocol::parseResponse` → route to UI.
3. **Send flow**: UI emits signals → MainWindow calls `IProtocol::buildRequest` → `ITransport::send`.
4. **Timeout**: MainWindow manages the 3s timeout timer (moved from TcpClient).
5. **Mock mode**: Creates MockServer with current `IProtocol`.

The `setupConnections()` method connects:
- Transport signals (`connected`, `disconnected`, `dataReceived`, `errorOccurred`)
- RealtimeWidget's `pollRequest` signal → build request → send via transport
- CommandWidget's `sendCommand` signal → build request → send via transport
- Response routing: parse → update realtime/command tabs + log

- [ ] **Step 3: Update `GSK988Tool.pro`**

Add new files to SOURCES and HEADERS, add `serialport` to QT:

```
QT += core gui network widgets serialport

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/transport/tcptransport.cpp \
    src/network/mockserver.cpp \
    src/protocol/gsk988/gsk988protocol.cpp \
    src/protocol/gsk988/gsk988framebuilder.cpp \
    src/protocol/gsk988/gsk988widgetfactory.cpp \
    src/protocol/gsk988/gsk988realtimewidget.cpp \
    src/protocol/gsk988/gsk988commandwidget.cpp \
    src/protocol/gsk988/gsk988parsewidget.cpp \
    src/ui/logwidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/transport/itransport.h \
    src/transport/tcptransport.h \
    src/network/mockserver.h \
    src/protocol/iprotocol.h \
    src/protocol/gsk988/gsk988protocol.h \
    src/protocol/gsk988/gsk988framebuilder.h \
    src/protocol/gsk988/gsk988widgetfactory.h \
    src/protocol/gsk988/gsk988realtimewidget.h \
    src/protocol/gsk988/gsk988commandwidget.h \
    src/protocol/gsk988/gsk988parsewidget.h \
    src/ui/iprotocolwidgetfactory.h \
    src/ui/logwidget.h
```

- [ ] **Step 4: Delete old files**

Remove files that have been fully migrated:
- `src/network/tcpclient.h/.cpp`
- `src/protocol/framebuilder.h/.cpp`
- `src/protocol/gsk988protocol.h/.cpp` (old location, now in `gsk988/` subdirectory)
- `src/ui/realtimewidget.h/.cpp`
- `src/ui/commandwidget.h/.cpp`
- `src/ui/parsewidget.h/.cpp`

- [ ] **Step 5: Build and verify**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`
Expected: BUILD SUCCESS

Run also MinGW build to verify both compilers.

- [ ] **Step 6: Test GSK988 functionality**

Launch the rebuilt app. Verify:
1. Connect to real GSK988 device (or mock) — data flows correctly
2. Real-time polling works with auto-refresh
3. Command sending works
4. Parse tool works
5. Log persists across operations

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "refactor: MainWindow as multi-protocol orchestrator, remove old files"
```

---

### Task 7: Add SerialTransport Stub

**Files:**
- Create: `src/transport/serialtransport.h`
- Create: `src/transport/serialtransport.cpp`
- Modify: `GSK988Tool.pro` (add files)
- Modify: `src/mainwindow.cpp` (register in transport combo)

A functional stub — creates config UI (COM port + baud rate), but doesn't actually open the serial port yet. This lets the user see the transport switch working.

- [ ] **Step 1: Create `src/transport/serialtransport.h`**

```cpp
#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "itransport.h"
#include <QComboBox>
#include <QSpinBox>

class SerialTransport : public ITransport {
    Q_OBJECT
public:
    explicit SerialTransport(QObject* parent = nullptr);
    ~SerialTransport();

    QString name() const override { return "串口"; }
    QWidget* createConfigWidget() override;
    void connectToHost() override;
    void disconnectFromHost() override;
    bool isConnected() const override;
    void send(const QByteArray& data) override;

private:
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    bool m_connected = false;
};

#endif // SERIALTRANSPORT_H
```

- [ ] **Step 2: Create `src/transport/serialtransport.cpp`**

```cpp
#include "serialtransport.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSerialPortInfo>

SerialTransport::SerialTransport(QObject* parent)
    : ITransport(parent)
    , m_portCombo(nullptr)
    , m_baudCombo(nullptr)
{
}

SerialTransport::~SerialTransport() {}

QWidget* SerialTransport::createConfigWidget()
{
    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QLabel("端口:"));
    m_portCombo = new QComboBox;
    for (const auto& info : QSerialPortInfo::availablePorts())
        m_portCombo->addItem(info.portName());
    layout->addWidget(m_portCombo);

    layout->addWidget(new QLabel("波特率:"));
    m_baudCombo = new QComboBox;
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200"});
    m_baudCombo->setCurrentText("115200");
    layout->addWidget(m_baudCombo);

    return widget;
}

void SerialTransport::connectToHost()
{
    // Stub: actual serial connection will be implemented later
    m_connected = true;
    emit connected();
}

void SerialTransport::disconnectFromHost()
{
    m_connected = false;
    emit disconnected();
}

bool SerialTransport::isConnected() const { return m_connected; }

void SerialTransport::send(const QByteArray& /*data*/)
{
    // Stub: no-op
}
```

- [ ] **Step 3: Register in MainWindow transport combo and .pro**

In MainWindow constructor, after creating transport combo:
```cpp
m_transportCombo->addItems({"TCP", "串口"});
```

In `onTransportChanged()`, add serial case:
```cpp
case 1: m_transport = new SerialTransport(this); break;
```

In `GSK988Tool.pro` SOURCES add: `src/transport/serialtransport.cpp`
In `GSK988Tool.pro` HEADERS add: `src/transport/serialtransport.h`

- [ ] **Step 4: Build and verify**

Run: `cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1 | tail -5`

- [ ] **Step 5: Test transport switching**

Launch app. Switch transport combo from TCP to 串口 — config area should change from IP+Port to COM+波特率. Switch back to TCP — config area restores.

- [ ] **Step 6: Commit**

```bash
git add src/transport/serialtransport.h src/transport/serialtransport.cpp GSK988Tool.pro
git commit -m "feat: add SerialTransport stub with config UI"
```

---

### Task 8: Final Cleanup and Verification

**Files:**
- Modify: `GSK988Tool.pro` (remove any remaining old file references)
- Verify: `build_msvc.bat` still works

- [ ] **Step 1: Verify .pro file is clean**

Ensure no references to deleted files remain:
- No `src/network/tcpclient.cpp` or `.h`
- No `src/protocol/framebuilder.cpp` or `.h`
- No `src/protocol/gsk988protocol.cpp` or `.h` (old location)
- No `src/ui/realtimewidget`, `commandwidget`, `parsewidget` (old location)

- [ ] **Step 2: Clean build with MSVC**

```bash
cmd.exe //c "D:\xieyi\rebuild_msvc.bat" 2>&1
```
Expected: BUILD SUCCESS, no warnings about missing files.

- [ ] **Step 3: Full functionality test**

1. Launch app → toolbar shows: [协议: GSK988 ▼] [传输: TCP ▼] [IP+Port] [模式] [连接]
2. Switch 传输 to 串口 → config changes to COM+波特率
3. Switch back to TCP → config restores
4. Connect in Mock mode → real-time polling works
5. Send command tab → works
6. Parse tab → works
7. Log tab → persists across all operations

- [ ] **Step 4: Commit final state**

```bash
git add -A
git commit -m "chore: cleanup after multi-protocol refactoring"
```

---

## Summary

| Task | Description | Key Files |
|------|-------------|-----------|
| 1 | Define interfaces | 3 new headers |
| 2 | TcpTransport | transport/tcptransport.h/.cpp |
| 3 | GSK988 protocol plugin | protocol/gsk988/ (4 files) |
| 4 | GSK988 UI + WidgetFactory | protocol/gsk988/ (7 files) |
| 5 | Generic MockServer | network/mockserver.h/.cpp |
| 6 | MainWindow refactor | mainwindow.h/.cpp, .pro |
| 7 | SerialTransport stub | transport/serialtransport.h/.cpp |
| 8 | Cleanup + verify | all |

**After this plan:** Adding a new protocol (e.g. Modbus TCP) requires only:
1. Create `src/protocol/modbus/` with `modbusprotocol.h/.cpp` implementing `IProtocol`
2. Create `src/protocol/modbus/modbuswidgetfactory.h/.cpp` + 3 UI widgets
3. Register in MainWindow's protocol combo
4. Add to .pro
