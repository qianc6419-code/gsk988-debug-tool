# Modbus TCP Protocol Plugin Design

## Context

GSK988 debug tool v2.0 已完成插件架构（IProtocol / IProtocolWidgetFactory / ITransport），目前只有 GSK988 一个协议实现。本设计将 Modbus TCP 作为第二个协议插件加入，支持 Modbus TCP 完整标准功能码。

## Modbus TCP 帧结构

### ADU（Application Data Unit）

```
MBAP Header (7 bytes):
  Transaction ID  : 2 bytes (uint16, 大端) — 匹配请求/响应
  Protocol ID     : 2 bytes (0x0000)       — Modbus 协议固定值
  Length           : 2 bytes (uint16, 大端) — 后续字节数 = Unit ID(1) + PDU(n)
  Unit ID         : 1 byte                 — 从站地址

PDU (n bytes):
  Function Code   : 1 byte
  Data            : n-1 bytes
```

无起始/结束标志符，帧边界由 Length 字段决定。

### extractFrame 逻辑

```
1. buffer < 6 字节 → 等待更多数据
2. 读取 offset 4 的 Length 字段 (uint16 大端)
3. totalFrame = 6 + Length
4. buffer < totalFrame → 等待更多数据
5. 取出前 totalFrame 字节为一帧，从 buffer 移除
```

### 异常响应

功能码最高位被置 1（即 functionCode | 0x80），后跟 1 字节异常码：
- 01: 非法功能码
- 02: 非法数据地址
- 03: 非法数据值
- 04: 从站故障
- 05: 确认（长时间操作）
- 06: 从站忙
- 07: 负确认
- 08: 内存奇偶错误

## 支持的功能码（完整标准）

### 基本读写（Bit/Word 访问）

| 功能码 | 名称 | 请求参数 | 响应数据 |
|--------|------|---------|---------|
| 0x01 | Read Coils | 起始地址(2B) + 数量(2B, 1-2000) | 字节数(1B) + 线圈状态(nB) |
| 0x02 | Read Discrete Inputs | 起始地址(2B) + 数量(2B, 1-2000) | 字节数(1B) + 输入状态(nB) |
| 0x03 | Read Holding Registers | 起始地址(2B) + 数量(2B, 1-125) | 字节数(1B) + 寄存器值(nB) |
| 0x04 | Read Input Registers | 起始地址(2B) + 数量(2B, 1-125) | 字节数(1B) + 寄存器值(nB) |
| 0x05 | Write Single Coil | 输出地址(2B) + 输出值(2B: 0xFF00/0x0000) | 回显请求 |
| 0x06 | Write Single Register | 寄存器地址(2B) + 寄存器值(2B) | 回显请求 |
| 0x0F | Write Multiple Coils | 起始地址(2B) + 输出数量(2B) + 字节数(1B) + 输出值(nB) | 起始地址(2B) + 输出数量(2B) |
| 0x10 | Write Multiple Registers | 起始地址(2B) + 寄存器数量(2B) + 字节数(1B) + 寄存器值(nB) | 起始地址(2B) + 寄存器数量(2B) |

### 诊断与状态

| 功能码 | 名称 | 请求参数 | 响应数据 |
|--------|------|---------|---------|
| 0x07 | Read Exception Status | 无 | 异常状态(1B) |
| 0x08 | Diagnostics | 子功能(2B) + 数据(nB) | 子功能(2B) + 数据(nB) |
| 0x0B | Get Comm Event Counter | 无 | 状态(2B) + 事件计数(2B) |
| 0x0C | Get Comm Event Log | 无 | 状态(2B) + 事件计数(2B) + 消息计数(2B) + 事件(nB) |

### 文件记录

| 功能码 | 名称 | 请求参数 | 响应数据 |
|--------|------|---------|---------|
| 0x14 | Read File Record | 字节数(1B) + 引用组(n × 7B) | 文件响应数据 |
| 0x15 | Write File Record | 完整引用组数据 | 回显请求 |

### 其他

| 功能码 | 名称 | 请求参数 | 响应数据 |
|--------|------|---------|---------|
| 0x11 | Report Server ID | 无 | 服务器ID数据 |
| 0x17 | Read/Write Multiple Registers | 读起始地址(2B) + 读数量(2B) + 写起始地址(2B) + 写数量(2B) + 写字节数(1B) + 写数据(nB) | 读字节数(1B) + 读数据(nB) |
| 0x2B | Encapsulated Interface Transport | MEI类型(1B) + 数据(nB) | MEI类型(1B) + 数据(nB) |

## 文件组织

```
src/protocol/modbus/
  modbusprotocol.h/.cpp       # IProtocol 实现
  modbusframebuilder.h/.cpp   # MBAP 头构建 + PDU 编解码
  modbuswidgetfactory.h/.cpp  # IProtocolWidgetFactory 实现
  modbusrealtimewidget.h/.cpp # 实时数据轮询 Tab
  modbuscommandwidget.h/.cpp  # 指令发送 Tab
  modbusparsewidget.h/.cpp    # 手动帧解析 Tab
```

## 核心类设计

### ModbusProtocol

```cpp
class ModbusProtocol : public IProtocol {
    Q_OBJECT
public:
    explicit ModbusProtocol(QObject* parent = nullptr);

    QString name() const override { return "Modbus TCP"; }
    QString version() const override { return "1.0"; }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

private:
    // Transaction ID 自增器
    mutable quint16 m_transactionId = 0;
    quint16 nextTransactionId() const;

    // 功能码定义表
    static QVector<ProtocolCommand> s_commands;
};
```

### ModbusFrameBuilder

负责 MBAP 头组装和 PDU 字段编解码，纯工具类（静态方法）。

```cpp
class ModbusFrameBuilder {
public:
    // 组帧：MBAP头 + PDU
    static QByteArray buildFrame(quint16 txnId, quint8 unitId,
                                 quint8 funcCode, const QByteArray& pduData);

    // MBAP 头解析
    static quint16 parseTransactionId(const QByteArray& frame);
    static quint16 parseProtocolId(const QByteArray& frame);
    static quint16 parseLength(const QByteArray& frame);
    static quint8  parseUnitId(const QByteArray& frame);
    static quint8  parseFunctionCode(const QByteArray& frame);

    // 参数编码辅助
    static QByteArray encodeAddress(quint16 addr);      // 大端 2B
    static QByteArray encodeQuantity(quint16 qty);       // 大端 2B
    static QByteArray encodeValue(quint16 val);          // 大端 2B
    static QByteArray encodeCoilValues(const QVector<bool>& coils);
    static QByteArray encodeRegisterValues(const QVector<quint16>& regs);

    // 参数解码辅助
    static quint16 decodeUInt16(const QByteArray& data, int offset);
    static QVector<bool>   decodeCoils(const QByteArray& data, int count);
    static QVector<quint16> decodeRegisters(const QByteArray& data, int count);
};
```

## UI 设计

### ModbusRealtimeWidget — 实时数据轮询

用户配置轮询列表（地址 + 功能码 + 数量），自动循环读取并显示结果。

布局：
```
┌─ 轮询配置 ─────────────────────────────────────────┐
│ [起始地址: 0x0000] [功能码: 03▼] [数量: 10] [+添加] │
│ ┌─────────────────────────────────────────────────┐ │
│ │ 地址   功能码    名称     数量    操作           │ │
│ │ 0x0000  03(读保持) 温度     1     [删除]         │ │
│ │ 0x0001  03(读保持) 压力     1     [删除]         │ │
│ └─────────────────────────────────────────────────┘ │
│ [☑自动刷新] [间隔: 1000ms▼] [手动刷新]              │
└─────────────────────────────────────────────────────┘
┌─ 轮询结果 ─────────────────────────────────────────┐
│ ┌─────────────────────────────────────────────────┐ │
│ │ 地址     类型      原始值    解析值              │ │
│ │ 0x0000   UINT16    0x0172    370               │ │
│ │ 0x0001   INT16     0xFF9C    -100              │ │
│ │ 0x0002   FLOAT32   0x41A0    20.0 (跨2寄存器)  │ │
│ └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
┌─ Hex 收发记录 ─────────────────────────────────────┐
│ >> TX: 00 01 00 00 00 06 01 03 00 00 00 0A         │
│ << RX: 00 01 00 00 00 17 01 03 14 00 0A ...        │
└─────────────────────────────────────────────────────┘
```

轮询逻辑：
- PollItem = { funcCode, startAddr, quantity, name, dataType }
- 自动刷新时按顺序循环发送每个 PollItem，收到响应后更新表格
- DataType 支持：UINT16, INT16, UINT32, INT32, FLOAT32, BOOL
- FLOAT32 占 2 个连续寄存器（大端寄存器对）

### ModbusCommandWidget — 指令发送

左侧：功能码列表（可搜索/分类过滤）。右侧：参数输入 + 结果展示。

布局：
```
┌─ 搜索/过滤 ──────────────────────────────────────────┐
│ [搜索...]  [分类: 全部▼]                              │
├───────────────────┬──────────────────────────────────┤
│ 功能码列表        │  参数配置                          │
│                   │                                   │
│ > 01 读线圈       │  功能: 03 Read Holding Registers  │
│   02 读离散输入   │  Unit ID: [1]                     │
│ * 03 读保持寄存器 │  起始地址: [0x0000]               │
│   04 读输入寄存器 │  数量: [10]                       │
│   05 写单线圈     │  [发送]                           │
│   06 写单寄存器   │                                   │
│   ...             │  ── 响应 ──                       │
│                   │  Transaction: 0x0001              │
│                   │  Unit: 01  Func: 03               │
│                   │  数据: 00 0A 01 72 ...            │
│                   │  解析: [370, 370, ...]            │
└───────────────────┴──────────────────────────────────┘
```

分类：
- 基本读写 (01-06, 0F, 10)
- 诊断状态 (07, 08, 0B, 0C)
- 文件记录 (14, 15)
- 其他 (11, 17, 2B)

参数输入根据功能码动态变化：
- 读操作：起始地址 + 数量
- 写单个：地址 + 值
- 写多个：起始地址 + 数量 + 值列表
- Read/Write Multiple：读地址 + 读数量 + 写地址 + 写数量 + 写值列表

### ModbusParseWidget — 手动帧解析

输入 hex 字符串，逐字段解析 MBAP 头 + PDU。

布局：
```
┌─ 输入 ──────────────────────────────────────────────┐
│ 00 01 00 00 00 06 01 03 00 00 00 0A                  │
│                                          [解析]      │
└─────────────────────────────────────────────────────┘
┌─ 解析结果 ──────────────────────────────────────────┐
│ ── MBAP Header ──                                   │
│   Transaction ID : 0x0001 (1)                       │
│   Protocol ID    : 0x0000 (Modbus)                  │
│   Length          : 0x0006 (6)                       │
│   Unit ID         : 0x01 (1)                        │
│ ── PDU ──                                           │
│   Function Code : 0x03 (Read Holding Registers)     │
│   Start Address : 0x0000 (0)                        │
│   Quantity       : 0x000A (10)                      │
│ ── 摘要 ──                                          │
│   请求: 从 Unit 1 读取 10 个保持寄存器，起始地址 0   │
└─────────────────────────────────────────────────────┘
```

自动检测请求/响应/异常：
- 请求：PDU 数据长度匹配请求格式
- 响应：PDU 数据包含字节数 + 数据
- 异常：功能码 & 0x80 != 0

## Mock Server 行为

ModbusProtocol::mockResponse 实现：

- 维护模拟内存：256 字节线圈区 + 256 字节离散输入区 + 256 个保持寄存器 + 256 个输入寄存器
- 读操作：返回模拟内存中对应区域的数据
- 写操作：写入模拟内存，返回标准响应
- 诊断：返回固定响应（echo 子功能 0x0000，clear counters 返回成功等）
- Report Server ID：返回 {0xFF, 0xFF, 0x00}（服务器ID 255，运行状态正常，附加数据 0x00）
- 异常：无效功能码返回 0x81 + 01，地址越界返回 0x8x + 02

## MainWindow 集成

在 MainWindow::switchProtocol 中增加 Modbus TCP 选项：

```cpp
// protocolCombo 增加 "Modbus TCP" 选项
// switchProtocol 中:
case 1: // Modbus TCP
    m_protocol = new ModbusProtocol(this);
    m_widgetFactory = new ModbusWidgetFactory();
    break;
```

MockServer 已泛型化（通过 IProtocol 指针），无需修改。

## 数据格式约定

- 地址显示：hex 格式（0x0000）
- 数值显示：可选 hex/decimal（寄存器值），默认 decimal
- 所有寄存器值按大端解析（Modbus 标准）
- 字节序遵循 Modbus 规范：事务ID、协议ID、长度、地址、数量、寄存器值均为大端

## .pro 文件变更

```
SOURCES += \
    src/protocol/modbus/modbusprotocol.cpp \
    src/protocol/modbus/modbusframebuilder.cpp \
    src/protocol/modbus/modbuswidgetfactory.cpp \
    src/protocol/modbus/modbusrealtimewidget.cpp \
    src/protocol/modbus/modbuscommandwidget.cpp \
    src/protocol/modbus/modbusparsewidget.cpp

HEADERS += \
    src/protocol/modbus/modbusprotocol.h \
    src/protocol/modbus/modbusframebuilder.h \
    src/protocol/modbus/modbuswidgetfactory.h \
    src/protocol/modbus/modbusrealtimewidget.h \
    src/protocol/modbus/modbuscommandwidget.h \
    src/protocol/modbus/modbusparsewidget.h
```

无新 Qt 模块依赖（已含 network）。

## 验证

1. MSVC 编译通过
2. 协议切换到 Modbus TCP 不崩溃，三个 Tab 正常显示
3. Mock 模式下：读保持寄存器 (03) 返回模拟数据，写单寄存器 (06) 写入并回显
4. 手动解析 Tab 能正确解析请求帧和响应帧（含异常帧）
5. 实时轮询 Tab 能配置多个轮询项并循环读取
6. 指令发送 Tab 所有功能码参数输入正确，异常响应有错误提示
7. 切回 GSK988 功能不受影响
