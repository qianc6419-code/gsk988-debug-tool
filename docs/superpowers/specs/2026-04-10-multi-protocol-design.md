# Multi-Protocol Support Design

## Context

GSK988 debug tool currently supports only one protocol (GSK988) with hardcoded TCP transport. Need to support multiple industrial protocols (Modbus TCP, FANUC Focas, Siemens S7, etc.) with different transports (TCP, serial, UDP, etc.). New protocols will be added incrementally.

## Architecture

Plugin-style protocol architecture with three abstraction layers. Each protocol is a self-contained module implementing unified interfaces. MainWindow orchestrates via a protocol switcher.

### Layers

```
┌─────────────────────────────────────┐
│           MainWindow                │
│  [协议选择] [传输选择] [连接配置]     │
│  TabWidget: 实时数据 | 指令 | 解析 | 日志│
└────────┬───────────────────────────┘
         │
    ┌────┴────┐
    │Protocol │── IProtocol
    │ Widget  │── IProtocolWidgetFactory
    │ Factory │
    └────┬────┘
         │
    ┌────┴────┐
    │Protocol │── IProtocol
    │         │   buildRequest / parseResponse / extractFrame
    └────┬────┘
         │
    ┌────┴────┐
    │Transport│── ITransport
    │         │   connect / send / receive raw bytes
    └─────────┘
```

Data flow: Transport receives raw bytes -> Protocol extracts frames and parses -> UI displays results. LogWidget is shared across all protocols.

## Interfaces

### ITransport (`src/transport/itransport.h`)

```cpp
class ITransport : public QObject {
    Q_OBJECT
public:
    virtual ~ITransport() = default;
    virtual QString name() const = 0;            // "TCP" / "串口"
    virtual QWidget* createConfigWidget() = 0;   // toolbar config area
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual void send(const QByteArray& data) = 0;
signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& msg);
};
```

Implementations:
- **TcpTransport** (`src/transport/tcptransport.h/.cpp`) — IP + port config, wraps QTcpSocket
- **SerialTransport** (`src/transport/serialtransport.h/.cpp`) — COM port + baud rate config, wraps QSerialPort (add `serialport` to QT modules in .pro)

### IProtocol (`src/protocol/iprotocol.h`)

```cpp
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
```

Note: `extractFrame` lives in the protocol layer because frame delimiters are protocol-specific (GSK988 uses 0x93 0x00 head + 0x55 0xAA tail; Modbus uses MBAP header with length field).

### IProtocolWidgetFactory (`src/ui/iprotocolwidgetfactory.h`)

```cpp
class IProtocolWidgetFactory {
public:
    virtual ~IProtocolWidgetFactory() = default;
    virtual QWidget* createRealtimeWidget() = 0;
    virtual QWidget* createCommandWidget() = 0;
    virtual QWidget* createParseWidget() = 0;
};
```

## File Organization

```
src/
  transport/
    itransport.h
    tcptransport.h/.cpp          # QHostInfo + port
    serialtransport.h/.cpp       # QSerialPort
  protocol/
    iprotocol.h
    gsk988/
      gsk988protocol.h/.cpp      # refactored from current code
      gsk988framebuilder.h/.cpp  # refactored from current FrameBuilder
      gsk988widgetfactory.h/.cpp
      gsk988realtimewidget.h/.cpp
      gsk988commandwidget.h/.cpp
      gsk988parsewidget.h/.cpp
    modbus/
      modbusprotocol.h/.cpp
      modbuswidgetfactory.h/.cpp
      modbusrealtimewidget.h/.cpp
      modbuscommandwidget.h/.cpp
      modbusparsewidget.h/.cpp
  ui/
    logwidget.h/.cpp             # shared across protocols
  mainwindow.h/.cpp              # refactored as orchestrator
  main.cpp
```

## MainWindow Refactoring

### Toolbar

```
[协议: GSK988 ▼] | [传输: TCP ▼] [动态配置区] | [模式: 真实/Mock ▼] [连接] [●状态]
```

- Protocol combo: selects IProtocol + IProtocolWidgetFactory
- Transport combo: selects ITransport (TCP / 串口)
- Dynamic config area: replaced by `ITransport::createConfigWidget()` when transport changes
- Mode combo: real device / mock (mock uses protocol's mockResponse). Mock only available with TCP transport; hidden/disabled when transport is serial

### Tab Management

Tabs 0-2 are protocol-specific (replaced on protocol switch). Tab 3 (通讯日志) is persistent.

Protocol switch flow:
1. Disconnect current transport if connected
2. Remove tabs 0-2 from TabWidget, delete old widgets
3. Create new IProtocol + IWidgetFactory
4. Call factory->createRealtimeWidget/CommandWidget/ParseWidget
5. Insert as tabs 0-2
6. Reconnect signals

### Mock Server Refactoring

Current MockServer is GSK988-specific. Refactor to be generic:
- MockServer takes an IProtocol pointer
- On receiving data: extracts frames via IProtocol::extractFrame, generates response via IProtocol::mockResponse

## GSK988 Refactoring

Current code moves into `protocol/gsk988/` with minimal changes:
- `FrameBuilder` -> `Gsk988FrameBuilder` (internal to GSK988 protocol)
- `Gsk988Protocol` implements `IProtocol`
- UI widgets renamed with Gsk988 prefix, keep existing layout and logic
- `TcpClient` logic splits into `TcpTransport` (raw bytes) + `Gsk988Protocol::extractFrame` (framing)

## Implementation Scope (This Round)

1. Define interfaces: ITransport, IProtocol, IProtocolWidgetFactory
2. Implement TcpTransport (refactored from TcpClient)
3. Refactor GSK988 into plugin structure
4. Refactor MainWindow as orchestrator
5. Implement SerialTransport (stub for now, UI ready)
6. Modbus TCP protocol: implement in a future iteration after user provides spec

## Verification

1. Build with MinGW and MSVC — both must compile
2. GSK988 functionality unchanged after refactoring (real device + mock mode)
3. Protocol combo switchable at runtime without crash
4. Transport combo switchable (TCP works, Serial shows config UI)
5. LogWidget persists across protocol switches
