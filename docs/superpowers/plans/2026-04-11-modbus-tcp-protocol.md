# Modbus TCP Protocol Plugin Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Modbus TCP as the second protocol plugin to the multi-protocol debug tool, supporting the complete Modbus TCP standard function codes.

**Architecture:** Follow the existing plugin pattern — IProtocol + IProtocolWidgetFactory + ITransport. All Modbus-specific code lives in `src/protocol/modbus/`. MainWindow gains a second protocol combo entry. MockServer is already generic and needs no changes.

**Tech Stack:** C++14, Qt 5 (network, widgets, serialport)

---

## Task 1: ModbusFrameBuilder

**Files:**
- Create: `src/protocol/modbus/modbusframebuilder.h`
- Create: `src/protocol/modbus/modbusframebuilder.cpp`

- [ ] **Step 1: Create modbusframebuilder.h**

```cpp
#ifndef MODBUSFRAMEBUILDER_H
#define MODBUSFRAMEBUILDER_H

#include <QByteArray>
#include <QVector>

class ModbusFrameBuilder
{
public:
    // Build a complete Modbus TCP ADU frame
    // txnId: Transaction ID (auto-managed by ModbusProtocol)
    // unitId: Slave address (Unit ID)
    // funcCode: Modbus function code
    // pduData: PDU data after function code
    static QByteArray buildFrame(quint16 txnId, quint8 unitId,
                                 quint8 funcCode, const QByteArray& pduData);

    // MBAP header field extraction
    static quint16 parseTransactionId(const QByteArray& frame);
    static quint16 parseProtocolId(const QByteArray& frame);
    static quint16 parseLength(const QByteArray& frame);
    static quint8  parseUnitId(const QByteArray& frame);
    static quint8  parseFunctionCode(const QByteArray& frame);

    // Big-endian parameter encoding
    static QByteArray encodeAddress(quint16 addr);
    static QByteArray encodeQuantity(quint16 qty);
    static QByteArray encodeValue(quint16 val);
    static QByteArray encodeCoilValues(const QVector<bool>& coils);
    static QByteArray encodeRegisterValues(const QVector<quint16>& regs);

    // Big-endian parameter decoding
    static quint16 decodeUInt16(const QByteArray& data, int offset);
    static quint32 decodeUInt32(const QByteArray& data, int offset);
    static QVector<bool>    decodeCoils(const QByteArray& data, int offset, int count);
    static QVector<quint16> decodeRegisters(const QByteArray& data, int offset, int count);

    // Check if function code indicates an exception response
    static bool isException(quint8 funcCode);
};

#endif // MODBUSFRAMEBUILDER_H
```

- [ ] **Step 2: Create modbusframebuilder.cpp**

```cpp
#include "modbusframebuilder.h"

QByteArray ModbusFrameBuilder::buildFrame(quint16 txnId, quint8 unitId,
                                           quint8 funcCode, const QByteArray& pduData)
{
    QByteArray frame;
    // MBAP header (7 bytes) + PDU
    quint16 length = 1 + 1 + static_cast<quint16>(pduData.size()); // unitId + funcCode + data
    frame.reserve(7 + pduData.size());

    // Transaction ID (big-endian)
    frame.append(static_cast<char>((txnId >> 8) & 0xFF));
    frame.append(static_cast<char>(txnId & 0xFF));

    // Protocol ID (always 0x0000 for Modbus)
    frame.append('\x00');
    frame.append('\x00');

    // Length (big-endian)
    frame.append(static_cast<char>((length >> 8) & 0xFF));
    frame.append(static_cast<char>(length & 0xFF));

    // Unit ID
    frame.append(static_cast<char>(unitId));

    // Function code
    frame.append(static_cast<char>(funcCode));

    // PDU data
    frame.append(pduData);

    return frame;
}

quint16 ModbusFrameBuilder::parseTransactionId(const QByteArray& frame)
{
    if (frame.size() < 2) return 0;
    return (static_cast<quint8>(frame[0]) << 8) | static_cast<quint8>(frame[1]);
}

quint16 ModbusFrameBuilder::parseProtocolId(const QByteArray& frame)
{
    if (frame.size() < 4) return 0;
    return (static_cast<quint8>(frame[2]) << 8) | static_cast<quint8>(frame[3]);
}

quint16 ModbusFrameBuilder::parseLength(const QByteArray& frame)
{
    if (frame.size() < 6) return 0;
    return (static_cast<quint8>(frame[4]) << 8) | static_cast<quint8>(frame[5]);
}

quint8 ModbusFrameBuilder::parseUnitId(const QByteArray& frame)
{
    if (frame.size() < 7) return 0;
    return static_cast<quint8>(frame[6]);
}

quint8 ModbusFrameBuilder::parseFunctionCode(const QByteArray& frame)
{
    if (frame.size() < 8) return 0;
    return static_cast<quint8>(frame[7]);
}

QByteArray ModbusFrameBuilder::encodeAddress(quint16 addr)
{
    QByteArray ba;
    ba.append(static_cast<char>((addr >> 8) & 0xFF));
    ba.append(static_cast<char>(addr & 0xFF));
    return ba;
}

QByteArray ModbusFrameBuilder::encodeQuantity(quint16 qty)
{
    QByteArray ba;
    ba.append(static_cast<char>((qty >> 8) & 0xFF));
    ba.append(static_cast<char>(qty & 0xFF));
    return ba;
}

QByteArray ModbusFrameBuilder::encodeValue(quint16 val)
{
    QByteArray ba;
    ba.append(static_cast<char>((val >> 8) & 0xFF));
    ba.append(static_cast<char>(val & 0xFF));
    return ba;
}

QByteArray ModbusFrameBuilder::encodeCoilValues(const QVector<bool>& coils)
{
    int byteCount = (coils.size() + 7) / 8;
    QByteArray bytes(byteCount, '\0');
    for (int i = 0; i < coils.size(); ++i) {
        if (coils[i])
            bytes[i / 8] |= (1 << (i % 8));
    }
    return bytes;
}

QByteArray ModbusFrameBuilder::encodeRegisterValues(const QVector<quint16>& regs)
{
    QByteArray ba;
    ba.reserve(regs.size() * 2);
    for (quint16 reg : regs) {
        ba.append(static_cast<char>((reg >> 8) & 0xFF));
        ba.append(static_cast<char>(reg & 0xFF));
    }
    return ba;
}

quint16 ModbusFrameBuilder::decodeUInt16(const QByteArray& data, int offset)
{
    if (offset + 2 > data.size()) return 0;
    return (static_cast<quint8>(data[offset]) << 8) | static_cast<quint8>(data[offset + 1]);
}

quint32 ModbusFrameBuilder::decodeUInt32(const QByteArray& data, int offset)
{
    if (offset + 4 > data.size()) return 0;
    return (static_cast<quint32>(static_cast<quint8>(data[offset])) << 24) |
           (static_cast<quint32>(static_cast<quint8>(data[offset + 1])) << 16) |
           (static_cast<quint32>(static_cast<quint8>(data[offset + 2])) << 8) |
           static_cast<quint32>(static_cast<quint8>(data[offset + 3]));
}

QVector<bool> ModbusFrameBuilder::decodeCoils(const QByteArray& data, int offset, int count)
{
    QVector<bool> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        int byteIdx = offset + i / 8;
        int bitIdx = i % 8;
        bool val = false;
        if (byteIdx < data.size())
            val = (static_cast<quint8>(data[byteIdx]) >> bitIdx) & 1;
        result.append(val);
    }
    return result;
}

QVector<quint16> ModbusFrameBuilder::decodeRegisters(const QByteArray& data, int offset, int count)
{
    QVector<quint16> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.append(decodeUInt16(data, offset + i * 2));
    }
    return result;
}

bool ModbusFrameBuilder::isException(quint8 funcCode)
{
    return (funcCode & 0x80) != 0;
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/modbus/modbusframebuilder.h src/protocol/modbus/modbusframebuilder.cpp
git commit -m "feat(modbus): add ModbusFrameBuilder for MBAP header and PDU encoding"
```

---

## Task 2: ModbusProtocol

**Files:**
- Create: `src/protocol/modbus/modbusprotocol.h`
- Create: `src/protocol/modbus/modbusprotocol.cpp`

- [ ] **Step 1: Create modbusprotocol.h**

```cpp
#ifndef MODBUSPROTOCOL_H
#define MODBUSPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

struct ModbusCommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

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

    // Internal: raw command table
    static QVector<ModbusCommandDef> allCommandsRaw();
    static ModbusCommandDef commandDefRaw(quint8 cmdCode);
    static QString interpretDataRaw(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);

private:
    mutable quint16 m_transactionId = 0;
    quint16 nextTransactionId() const;

    // Mock memory for mock server
    static QVector<bool> s_mockCoils;
    static QVector<bool> s_mockDiscreteInputs;
    static QVector<quint16> s_mockHoldingRegs;
    static QVector<quint16> s_mockInputRegs;
    static void initMockMemory();

    static QVector<ModbusCommandDef> s_commands;
};

#endif // MODBUSPROTOCOL_H
```

- [ ] **Step 2: Create modbusprotocol.cpp — Part 1: command table + IProtocol boilerplate**

```cpp
#include "modbusprotocol.h"
#include "modbusframebuilder.h"

// ========== Mock Memory (static) ==========

QVector<bool> ModbusProtocol::s_mockCoils;
QVector<bool> ModbusProtocol::s_mockDiscreteInputs;
QVector<quint16> ModbusProtocol::s_mockHoldingRegs;
QVector<quint16> ModbusProtocol::s_mockInputRegs;

void ModbusProtocol::initMockMemory()
{
    if (!s_mockCoils.isEmpty()) return;
    s_mockCoils.resize(65536, false);
    s_mockDiscreteInputs.resize(65536, false);
    s_mockHoldingRegs.resize(65536, 0);
    s_mockInputRegs.resize(65536, 0);

    // Pre-fill some interesting values for demo
    s_mockCoils[0] = true;
    s_mockCoils[1] = false;
    s_mockCoils[2] = true;
    s_mockDiscreteInputs[0] = true;
    s_mockDiscreteInputs[1] = true;
    for (int i = 0; i < 10; ++i)
        s_mockHoldingRegs[i] = static_cast<quint16>(100 + i * 10);
    s_mockInputRegs[0] = 1234;
    s_mockInputRegs[1] = 5678;
}

// ========== Command Definitions ==========

QVector<ModbusCommandDef> ModbusProtocol::s_commands;

static QVector<ModbusCommandDef> buildCommandTable()
{
    QVector<ModbusCommandDef> cmds;

    auto add = [&](quint8 code, const QString& name, const QString& cat,
                   const QString& reqP, const QString& respD) {
        cmds.append({code, name, cat, reqP, respD});
    };

    // 基本读写 (Bit/Word)
    add(0x01, "Read Coils",                   "基本读写", "起始地址(2B)+数量(2B,1-2000)",           "字节数(1B)+线圈状态(nB)");
    add(0x02, "Read Discrete Inputs",         "基本读写", "起始地址(2B)+数量(2B,1-2000)",           "字节数(1B)+输入状态(nB)");
    add(0x03, "Read Holding Registers",       "基本读写", "起始地址(2B)+数量(2B,1-125)",            "字节数(1B)+寄存器值(nB)");
    add(0x04, "Read Input Registers",         "基本读写", "起始地址(2B)+数量(2B,1-125)",            "字节数(1B)+寄存器值(nB)");
    add(0x05, "Write Single Coil",            "基本读写", "输出地址(2B)+输出值(2B:FF00/0000)",      "回显请求");
    add(0x06, "Write Single Register",        "基本读写", "寄存器地址(2B)+寄存器值(2B)",            "回显请求");
    add(0x0F, "Write Multiple Coils",         "基本读写", "起始地址(2B)+数量(2B)+字节数(1B)+值(nB)", "起始地址(2B)+数量(2B)");
    add(0x10, "Write Multiple Registers",     "基本读写", "起始地址(2B)+数量(2B)+字节数(1B)+值(nB)", "起始地址(2B)+数量(2B)");

    // 诊断与状态
    add(0x07, "Read Exception Status",        "诊断状态", "无",                                    "异常状态(1B)");
    add(0x08, "Diagnostics",                  "诊断状态", "子功能(2B)+数据(nB)",                    "子功能(2B)+数据(nB)");
    add(0x0B, "Get Comm Event Counter",       "诊断状态", "无",                                    "状态(2B)+事件计数(2B)");
    add(0x0C, "Get Comm Event Log",           "诊断状态", "无",                                    "状态(2B)+事件计数(2B)+消息计数(2B)+事件(nB)");

    // 文件记录
    add(0x14, "Read File Record",             "文件记录", "字节数(1B)+引用组(n×7B)",                "文件响应数据");
    add(0x15, "Write File Record",            "文件记录", "完整引用组数据",                          "回显请求");

    // 其他
    add(0x11, "Report Server ID",             "其他",     "无",                                    "服务器ID数据");
    add(0x17, "Read/Write Multiple Registers","其他",     "读地址(2B)+读数量(2B)+写地址(2B)+写数量(2B)+写字节数(1B)+写数据(nB)", "读字节数(1B)+读数据(nB)");
    add(0x2B, "Encapsulated Interface Transport", "其他", "MEI类型(1B)+数据(nB)",                 "MEI类型(1B)+数据(nB)");

    return cmds;
}

QVector<ModbusCommandDef> ModbusProtocol::allCommandsRaw()
{
    if (s_commands.isEmpty())
        s_commands = buildCommandTable();
    return s_commands;
}

ModbusCommandDef ModbusProtocol::commandDefRaw(quint8 cmdCode)
{
    auto cmds = allCommandsRaw();
    for (const auto& c : cmds) {
        if (c.cmdCode == cmdCode)
            return c;
    }
    return {cmdCode, "未知功能码", "其他", "", ""};
}

// ========== IProtocol: allCommands / commandDef ==========

QVector<ProtocolCommand> ModbusProtocol::allCommands() const
{
    QVector<ProtocolCommand> result;
    auto rawCmds = allCommandsRaw();
    result.reserve(rawCmds.size());
    for (const auto& c : rawCmds) {
        ProtocolCommand pc;
        pc.cmdCode = c.cmdCode;
        pc.name = c.name;
        pc.category = c.category;
        pc.paramDesc = c.requestParams;
        pc.respDesc = c.responseDesc;
        result.append(pc);
    }
    return result;
}

ProtocolCommand ModbusProtocol::commandDef(quint8 cmdCode) const
{
    ModbusCommandDef raw = commandDefRaw(cmdCode);
    ProtocolCommand pc;
    pc.cmdCode = raw.cmdCode;
    pc.name = raw.name;
    pc.category = raw.category;
    pc.paramDesc = raw.requestParams;
    pc.respDesc = raw.responseDesc;
    return pc;
}

// ========== Constructor ==========

ModbusProtocol::ModbusProtocol(QObject* parent)
    : IProtocol(parent)
{
}

quint16 ModbusProtocol::nextTransactionId() const
{
    return m_transactionId++;
}
```

- [ ] **Step 3: Add buildRequest / extractFrame / parseResponse to modbusprotocol.cpp**

Append to `modbusprotocol.cpp`:

```cpp
// ========== Build Request ==========

QByteArray ModbusProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    // params is expected to contain: unitId(1B) + pduData(nB)
    // If params is empty or has no unitId, default unitId=1
    quint8 unitId = 1;
    QByteArray pduData;
    if (params.size() >= 1) {
        unitId = static_cast<quint8>(params[0]);
        pduData = params.mid(1);
    }

    quint16 txnId = nextTransactionId();
    return ModbusFrameBuilder::buildFrame(txnId, unitId, cmdCode, pduData);
}

// ========== Extract Frame ==========

QByteArray ModbusProtocol::extractFrame(QByteArray& buffer)
{
    // Need at least 6 bytes for MBAP header
    if (buffer.size() < 6)
        return QByteArray();

    // Read Length field (big-endian, bytes 4-5)
    quint16 length = (static_cast<quint8>(buffer[4]) << 8) |
                     static_cast<quint8>(buffer[5]);

    int totalFrameSize = 6 + length; // MBAP header(6) + length

    if (buffer.size() < totalFrameSize)
        return QByteArray();

    QByteArray frame = buffer.left(totalFrameSize);
    buffer.remove(0, totalFrameSize);
    return frame;
}

// ========== Parse Response ==========

ParsedResponse ModbusProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse result;

    // Minimum frame: MBAP(6) + UnitID(1) + FuncCode(1) = 8 bytes
    if (frame.size() < 8) {
        result.isValid = false;
        return result;
    }

    // Verify Protocol ID is 0x0000
    quint16 protoId = ModbusFrameBuilder::parseProtocolId(frame);
    if (protoId != 0x0000) {
        result.isValid = false;
        return result;
    }

    quint8 funcCode = ModbusFrameBuilder::parseFunctionCode(frame);
    result.cmdCode = funcCode;

    // Check for exception response
    if (ModbusFrameBuilder::isException(funcCode)) {
        result.isValid = false;
        result.cmdCode = funcCode & 0x7F; // Store original function code
        // rawData contains the exception code (1 byte at offset 8)
        result.rawData = frame.mid(8);
        return result;
    }

    result.isValid = true;
    // rawData = PDU data after function code (offset 8)
    result.rawData = frame.mid(8);
    return result;
}
```

- [ ] **Step 4: Add interpretDataRaw to modbusprotocol.cpp**

Append to `modbusprotocol.cpp`:

```cpp
// ========== Interpret Data ==========

QString ModbusProtocol::interpretDataRaw(quint8 cmdCode, const QByteArray& data)
{
    if (data.isEmpty())
        return "(无附加数据)";

    auto hexDump = [](const QByteArray& ba) -> QString {
        QStringList hex;
        for (int i = 0; i < ba.size(); ++i)
            hex << QString("%1").arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return hex.join(" ");
    };

    switch (cmdCode) {
    case 0x01: case 0x02: {
        // Response: byteCount(1B) + coil/input states
        if (data.size() < 1) return hexDump(data);
        int byteCount = static_cast<quint8>(data[0]);
        int totalBits = byteCount * 8;
        auto bits = ModbusFrameBuilder::decodeCoils(data, 1, totalBits);
        QStringList vals;
        for (int i = 0; i < qMin(totalBits, 16); ++i)
            vals << QString::number(bits[i]);
        if (totalBits > 16) vals << "...";
        return QString("字节数: %1, 值: [%2]").arg(byteCount).arg(vals.join(", "));
    }
    case 0x03: case 0x04: {
        // Response: byteCount(1B) + register values
        if (data.size() < 1) return hexDump(data);
        int byteCount = static_cast<quint8>(data[0]);
        int regCount = byteCount / 2;
        auto regs = ModbusFrameBuilder::decodeRegisters(data, 1, regCount);
        QStringList vals;
        for (quint16 r : regs)
            vals << QString::number(r);
        return QString("字节数: %1, 寄存器值: [%2]").arg(byteCount).arg(vals.join(", "));
    }
    case 0x05: {
        // Echo: address(2B) + value(2B: FF00 or 0000)
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 val = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("地址: %1 (0x%2), 值: %3 (%4)")
                   .arg(addr).arg(addr, 4, 16, QChar('0')).toUpper()
                   .arg(val == 0xFF00 ? "ON" : "OFF")
                   .arg(val, 4, 16, QChar('0')).toUpper();
    }
    case 0x06: {
        // Echo: address(2B) + value(2B)
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 val = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("地址: %1 (0x%2), 值: %3 (0x%4)")
                   .arg(addr).arg(addr, 4, 16, QChar('0')).toUpper()
                   .arg(val).arg(val, 4, 16, QChar('0')).toUpper();
    }
    case 0x0F: {
        // Response: startAddress(2B) + quantity(2B)
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("起始地址: %1, 写入数量: %2").arg(addr).arg(qty);
    }
    case 0x10: {
        // Response: startAddress(2B) + quantity(2B)
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("起始地址: %1, 写入数量: %2").arg(addr).arg(qty);
    }
    case 0x07: {
        if (data.size() >= 1) {
            quint8 status = static_cast<quint8>(data[0]);
            return QString("异常状态: 0x%1").arg(status, 2, 16, QChar('0')).toUpper();
        }
        break;
    }
    case 0x08: {
        if (data.size() >= 2) {
            quint16 subFunc = ModbusFrameBuilder::decodeUInt16(data, 0);
            return QString("诊断子功能: 0x%1, 数据: %2")
                       .arg(subFunc, 4, 16, QChar('0')).toUpper()
                       .arg(hexDump(data.mid(2)));
        }
        break;
    }
    case 0x0B: {
        if (data.size() >= 4) {
            quint16 status = ModbusFrameBuilder::decodeUInt16(data, 0);
            quint16 count = ModbusFrameBuilder::decodeUInt16(data, 2);
            return QString("状态: 0x%1, 事件计数: %2")
                       .arg(status, 4, 16, QChar('0')).toUpper().arg(count);
        }
        break;
    }
    case 0x0C: {
        if (data.size() >= 6) {
            quint16 status = ModbusFrameBuilder::decodeUInt16(data, 0);
            quint16 events = ModbusFrameBuilder::decodeUInt16(data, 2);
            quint16 msgs = ModbusFrameBuilder::decodeUInt16(data, 4);
            return QString("状态: 0x%1, 事件: %2, 消息: %3")
                       .arg(status, 4, 16, QChar('0')).toUpper().arg(events).arg(msgs);
        }
        break;
    }
    case 0x11: {
        return QString("Server ID: %1").arg(hexDump(data));
    }
    case 0x14: {
        return QString("文件记录数据: %1").arg(hexDump(data));
    }
    case 0x15: {
        return QString("文件记录写入回显: %1").arg(hexDump(data));
    }
    case 0x17: {
        if (data.size() >= 1) {
            int byteCount = static_cast<quint8>(data[0]);
            auto regs = ModbusFrameBuilder::decodeRegisters(data, 1, byteCount / 2);
            QStringList vals;
            for (quint16 r : regs)
                vals << QString::number(r);
            return QString("读字节数: %1, 值: [%2]").arg(byteCount).arg(vals.join(", "));
        }
        break;
    }
    case 0x2B: {
        if (data.size() >= 1) {
            quint8 meiType = static_cast<quint8>(data[0]);
            return QString("MEI类型: 0x%1, 数据: %2")
                       .arg(meiType, 2, 16, QChar('0')).toUpper()
                       .arg(hexDump(data.mid(1)));
        }
        break;
    }
    default: {
        return QString("HEX: %1").arg(hexDump(data));
    }
    }
    return QString("(数据长度: %1字节)").arg(data.size());
}

QString ModbusProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    return interpretDataRaw(cmdCode, data);
}
```

- [ ] **Step 5: Add mockResponseData and mockResponse to modbusprotocol.cpp**

Append to `modbusprotocol.cpp`:

```cpp
// ========== Mock Response Data (internal) ==========

QByteArray ModbusProtocol::mockResponseData(quint8 cmdCode, const QByteArray& requestData)
{
    initMockMemory();
    QByteArray respData;

    switch (cmdCode) {
    case 0x01: {
        // Read Coils
        if (requestData.size() < 4) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        if (qty < 1 || qty > 2000) break;
        int byteCount = (qty + 7) / 8;
        QByteArray coilBytes(byteCount, '\0');
        for (int i = 0; i < qty && (addr + i) < s_mockCoils.size(); ++i) {
            if (s_mockCoils[addr + i])
                coilBytes[i / 8] |= (1 << (i % 8));
        }
        respData.append(static_cast<char>(byteCount));
        respData.append(coilBytes);
        break;
    }
    case 0x02: {
        // Read Discrete Inputs
        if (requestData.size() < 4) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        if (qty < 1 || qty > 2000) break;
        int byteCount = (qty + 7) / 8;
        QByteArray inputBytes(byteCount, '\0');
        for (int i = 0; i < qty && (addr + i) < s_mockDiscreteInputs.size(); ++i) {
            if (s_mockDiscreteInputs[addr + i])
                inputBytes[i / 8] |= (1 << (i % 8));
        }
        respData.append(static_cast<char>(byteCount));
        respData.append(inputBytes);
        break;
    }
    case 0x03: {
        // Read Holding Registers
        if (requestData.size() < 4) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        if (qty < 1 || qty > 125) break;
        int byteCount = qty * 2;
        respData.append(static_cast<char>(byteCount));
        for (int i = 0; i < qty && (addr + i) < s_mockHoldingRegs.size(); ++i) {
            quint16 val = s_mockHoldingRegs[addr + i];
            respData.append(ModbusFrameBuilder::encodeValue(val));
        }
        break;
    }
    case 0x04: {
        // Read Input Registers
        if (requestData.size() < 4) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        if (qty < 1 || qty > 125) break;
        int byteCount = qty * 2;
        respData.append(static_cast<char>(byteCount));
        for (int i = 0; i < qty && (addr + i) < s_mockInputRegs.size(); ++i) {
            quint16 val = s_mockInputRegs[addr + i];
            respData.append(ModbusFrameBuilder::encodeValue(val));
        }
        break;
    }
    case 0x05: {
        // Write Single Coil — echo request
        respData = requestData;
        if (requestData.size() >= 4) {
            quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
            quint16 val = ModbusFrameBuilder::decodeUInt16(requestData, 2);
            if (addr < s_mockCoils.size())
                s_mockCoils[addr] = (val == 0xFF00);
        }
        break;
    }
    case 0x06: {
        // Write Single Register — echo request
        respData = requestData;
        if (requestData.size() >= 4) {
            quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
            quint16 val = ModbusFrameBuilder::decodeUInt16(requestData, 2);
            if (addr < s_mockHoldingRegs.size())
                s_mockHoldingRegs[addr] = val;
        }
        break;
    }
    case 0x0F: {
        // Write Multiple Coils
        if (requestData.size() < 5) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        // requestData[3] = byteCount, requestData[4..] = coil values
        if (requestData.size() >= 5 + (qty + 7) / 8) {
            quint8 byteCount = static_cast<quint8>(requestData[3]);
            QByteArray coilData = requestData.mid(4, byteCount);
            for (int i = 0; i < qty && (addr + i) < s_mockCoils.size(); ++i) {
                int byteIdx = i / 8;
                int bitIdx = i % 8;
                bool val = (static_cast<quint8>(coilData[byteIdx]) >> bitIdx) & 1;
                s_mockCoils[addr + i] = val;
            }
        }
        respData.append(ModbusFrameBuilder::encodeAddress(addr));
        respData.append(ModbusFrameBuilder::encodeQuantity(qty));
        break;
    }
    case 0x10: {
        // Write Multiple Registers
        if (requestData.size() < 5) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        // requestData[3] = byteCount, requestData[4..] = register values
        if (requestData.size() >= 5 + qty * 2) {
            for (int i = 0; i < qty && (addr + i) < s_mockHoldingRegs.size(); ++i) {
                s_mockHoldingRegs[addr + i] = ModbusFrameBuilder::decodeUInt16(requestData, 5 + i * 2);
            }
        }
        respData.append(ModbusFrameBuilder::encodeAddress(addr));
        respData.append(ModbusFrameBuilder::encodeQuantity(qty));
        break;
    }
    case 0x07:
        respData.append('\x00'); // exception status = 0
        break;
    case 0x08: {
        // Diagnostics — echo back sub-function + data
        if (requestData.size() >= 4) {
            quint16 subFunc = ModbusFrameBuilder::decodeUInt16(requestData, 0);
            if (subFunc == 0x0000) {
                // Echo: return query data
                respData.append(requestData);
            } else {
                respData.append(requestData);
            }
        }
        break;
    }
    case 0x0B:
        respData.append(ModbusFrameBuilder::encodeValue(0x0000)); // status
        respData.append(ModbusFrameBuilder::encodeValue(42));     // event count
        break;
    case 0x0C:
        respData.append(ModbusFrameBuilder::encodeValue(0x0000)); // status
        respData.append(ModbusFrameBuilder::encodeValue(42));     // event count
        respData.append(ModbusFrameBuilder::encodeValue(100));    // message count
        respData.append('\x20');                                   // one event
        break;
    case 0x11:
        respData.append('\xFF'); // Server ID = 255
        respData.append('\xFF'); // Run indicator = ON
        respData.append('\x00'); // Additional data
        break;
    case 0x14:
        // Read File Record — minimal response
        respData.append('\x00'); // byte count
        break;
    case 0x15:
        // Write File Record — echo request
        respData = requestData;
        break;
    case 0x17: {
        // Read/Write Multiple Registers
        if (requestData.size() < 9) break;
        quint16 readAddr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 readQty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        quint16 writeAddr = ModbusFrameBuilder::decodeUInt16(requestData, 4);
        quint16 writeQty = ModbusFrameBuilder::decodeUInt16(requestData, 6);
        quint8 writeByteCount = static_cast<quint8>(requestData[8]);

        // Write registers
        if (requestData.size() >= 9 + writeByteCount) {
            for (int i = 0; i < writeQty && (writeAddr + i) < s_mockHoldingRegs.size(); ++i) {
                s_mockHoldingRegs[writeAddr + i] = ModbusFrameBuilder::decodeUInt16(requestData, 9 + i * 2);
            }
        }

        // Read registers
        int readByteCount = readQty * 2;
        respData.append(static_cast<char>(readByteCount));
        for (int i = 0; i < readQty && (readAddr + i) < s_mockHoldingRegs.size(); ++i) {
            respData.append(ModbusFrameBuilder::encodeValue(s_mockHoldingRegs[readAddr + i]));
        }
        break;
    }
    case 0x2B:
        // Encapsulated Interface Transport — minimal response
        if (requestData.size() >= 1) {
            respData.append(requestData[0]); // MEI type
            respData.append('\x00');         // no data
        }
        break;
    default:
        // Unknown — return exception (function code not supported)
        return QByteArray();
    }

    return respData;
}

// ========== Mock Response (IProtocol — returns complete frame) ==========

QByteArray ModbusProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    // requestData comes from parseResponse -> rawData, which is PDU data after funcCode
    // We need to build a response frame with matching transaction ID
    QByteArray respData = mockResponseData(cmdCode, requestData);
    if (respData.isEmpty()) {
        // Return exception response
        quint16 txnId = m_transactionId; // reuse current
        quint8 unitId = 1;
        QByteArray excData;
        excData.append('\x01'); // exception code: illegal function
        return ModbusFrameBuilder::buildFrame(txnId, unitId, cmdCode | 0x80, excData);
    }

    quint16 txnId = m_transactionId; // reuse current
    quint8 unitId = 1;
    return ModbusFrameBuilder::buildFrame(txnId, unitId, cmdCode, respData);
}
```

- [ ] **Step 6: Commit**

```bash
git add src/protocol/modbus/modbusprotocol.h src/protocol/modbus/modbusprotocol.cpp
git commit -m "feat(modbus): add ModbusProtocol with full standard function codes and mock memory"
```

---

## Task 3: ModbusParseWidget

**Files:**
- Create: `src/protocol/modbus/modbusparsewidget.h`
- Create: `src/protocol/modbus/modbusparsewidget.cpp`

- [ ] **Step 1: Create modbusparsewidget.h**

```cpp
#ifndef MODBUSPARSEWIDGET_H
#define MODBUSPARSEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class ModbusParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;
};

#endif // MODBUSPARSEWIDGET_H
```

- [ ] **Step 2: Create modbusparsewidget.cpp**

```cpp
#include "modbusparsewidget.h"
#include "modbusprotocol.h"
#include "modbusframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

ModbusParseWidget::ModbusParseWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ModbusParseWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Input section
    auto* inputLabel = new QLabel("输入 HEX 帧数据（支持空格、逗号、0x 前缀）:");
    m_hexInput = new QTextEdit;
    m_hexInput->setFontFamily("Consolas");
    m_hexInput->setMaximumHeight(100);
    m_hexInput->setPlaceholderText("00 01 00 00 00 06 01 03 00 00 00 0A");

    m_parseBtn = new QPushButton("解析");
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &ModbusParseWidget::doParse);

    auto* inputBar = new QHBoxLayout;
    inputBar->addWidget(m_hexInput, 3);
    inputBar->addWidget(m_parseBtn);

    mainLayout->addWidget(inputLabel);
    mainLayout->addLayout(inputBar);

    // Result section
    auto* resultLabel = new QLabel("解析结果:");
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 13px; }");

    mainLayout->addWidget(resultLabel);
    mainLayout->addWidget(m_resultDisplay, 1);
}

void ModbusParseWidget::doParse()
{
    QString text = m_hexInput->toPlainText();
    text.remove(' ');
    text.remove(',');
    text.remove('\n');
    text.remove('\r');
    text.remove("0x", Qt::CaseInsensitive);
    text.remove("0X");

    QByteArray frame = QByteArray::fromHex(text.toUtf8());
    if (frame.isEmpty()) {
        m_resultDisplay->setHtml("<span style='color:red;'>输入数据无效或为空</span>");
        return;
    }

    // Minimum Modbus TCP frame: MBAP(6) + UnitID(1) + FuncCode(1) = 8 bytes
    if (frame.size() < 8) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧太短：Modbus TCP 帧至少需要 8 字节（MBAP头7字节+功能码1字节）</span>");
        return;
    }

    // Verify Protocol ID
    quint16 protoId = ModbusFrameBuilder::parseProtocolId(frame);
    if (protoId != 0x0000) {
        m_resultDisplay->setHtml(QString("<span style='color:red;'>协议ID错误: 0x%1 (应为 0x0000)</span>")
                                     .arg(protoId, 4, 16, QChar('0')).toUpper());
        return;
    }

    auto coloredHex = [](const QByteArray& ba, const QString& color) -> QString {
        QStringList bytes;
        for (int i = 0; i < ba.size(); ++i)
            bytes << QString("<span style='color:%1;'>%2</span>")
                     .arg(color)
                     .arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return bytes.join(" ");
    };

    quint16 txnId = ModbusFrameBuilder::parseTransactionId(frame);
    quint16 length = ModbusFrameBuilder::parseLength(frame);
    quint8 unitId = ModbusFrameBuilder::parseUnitId(frame);
    quint8 funcCode = ModbusFrameBuilder::parseFunctionCode(frame);

    QString html;

    // MBAP Header
    html += "<b>── MBAP Header ──</b><br>";
    html += QString("  Transaction ID : <span style='color:#2196F3;'>0x%1</span> (%2)<br>")
                .arg(txnId, 4, 16, QChar('0')).toUpper().arg(txnId);
    html += QString("  Protocol ID    : <span style='color:#9E9E9E;'>0x%1</span> (Modbus)<br>")
                .arg(protoId, 4, 16, QChar('0')).toUpper();
    html += QString("  Length          : <span style='color:#FF9800;'>0x%1</span> (%2 字节)<br>")
                .arg(length, 4, 16, QChar('0')).toUpper().arg(length);
    html += QString("  Unit ID         : <span style='color:#4CAF50;'>0x%1</span> (%2)<br>")
                .arg(unitId, 2, 16, QChar('0')).toUpper().arg(unitId);

    // PDU
    html += "<b>── PDU ──</b><br>";

    bool isException = ModbusFrameBuilder::isException(funcCode);
    quint8 originalFunc = isException ? (funcCode & 0x7F) : funcCode;
    auto cmdDef = ModbusProtocol::commandDefRaw(originalFunc);

    if (isException) {
        html += QString("  Function Code : <span style='color:#F44336;'>0x%1</span> (异常! 原始: 0x%2 %3)<br>")
                    .arg(funcCode, 2, 16, QChar('0')).toUpper()
                    .arg(originalFunc, 2, 16, QChar('0')).toUpper()
                    .arg(cmdDef.name);
        if (frame.size() > 8) {
            quint8 excCode = static_cast<quint8>(frame[8]);
            QString excMsg;
            switch (excCode) {
            case 1: excMsg = "非法功能码"; break;
            case 2: excMsg = "非法数据地址"; break;
            case 3: excMsg = "非法数据值"; break;
            case 4: excMsg = "从站故障"; break;
            case 5: excMsg = "确认"; break;
            case 6: excMsg = "从站忙"; break;
            case 7: excMsg = "负确认"; break;
            case 8: excMsg = "内存奇偶错误"; break;
            default: excMsg = QString("未知(%1)").arg(excCode); break;
            }
            html += QString("  异常码: <span style='color:#F44336;'>0x%1</span> (%2)<br>")
                        .arg(excCode, 2, 16, QChar('0')).toUpper().arg(excMsg);
        }
    } else {
        html += QString("  Function Code : <span style='color:#4CAF50;'>0x%1</span> (%2)<br>")
                    .arg(funcCode, 2, 16, QChar('0')).toUpper().arg(cmdDef.name);

        QByteArray pduData = frame.mid(8);
        if (!pduData.isEmpty()) {
            html += QString("  数据: %1<br>").arg(coloredHex(pduData, "#9C27B0"));

            // Try semantic interpretation
            QString interp = ModbusProtocol::interpretDataRaw(funcCode, pduData);
            html += QString("  <b>语义:</b> %1<br>").arg(interp);
        }
    }

    // Summary
    html += "<b>── 摘要 ──</b><br>";
    if (isException) {
        html += QString("异常响应: Unit %1, 功能码 0x%2")
                    .arg(unitId).arg(originalFunc, 2, 16, QChar('0')).toUpper();
    } else {
        // Determine if this looks like a request or response
        bool looksLikeRequest = false;
        switch (funcCode) {
        case 0x01: case 0x02: case 0x03: case 0x04:
            looksLikeRequest = (frame.size() == 12); // request = MBAP(6)+UID(1)+FC(1)+addr(2)+qty(2) = 12
            break;
        case 0x05: case 0x06:
            looksLikeRequest = true; // same format for request/response
            break;
        case 0x0F: case 0x10:
            looksLikeRequest = (frame.size() > 13); // request has more data than response
            break;
        }
        if (looksLikeRequest && (funcCode == 0x01 || funcCode == 0x02 || funcCode == 0x03 || funcCode == 0x04)) {
            QByteArray pduData = frame.mid(8);
            if (pduData.size() >= 4) {
                quint16 addr = ModbusFrameBuilder::decodeUInt16(pduData, 0);
                quint16 qty = ModbusFrameBuilder::decodeUInt16(pduData, 2);
                html += QString("请求: 从 Unit %1 %2, 起始地址 %3, 数量 %4")
                            .arg(unitId).arg(cmdDef.name).arg(addr).arg(qty);
            }
        } else {
            html += QString("响应: Unit %1, 功能码 0x%2 (%3)")
                        .arg(unitId).arg(funcCode, 2, 16, QChar('0')).toUpper().arg(cmdDef.name);
        }
    }

    m_resultDisplay->setHtml(html);
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/modbus/modbusparsewidget.h src/protocol/modbus/modbusparsewidget.cpp
git commit -m "feat(modbus): add ModbusParseWidget for manual frame analysis"
```

---

## Task 4: ModbusCommandWidget

**Files:**
- Create: `src/protocol/modbus/modbuscommandwidget.h`
- Create: `src/protocol/modbus/modbuscommandwidget.cpp`

- [ ] **Step 1: Create modbuscommandwidget.h**

```cpp
#ifndef MODBUSCOMMANDWIDGET_H
#define MODBUSCOMMANDWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVector>
#include "protocol/iprotocol.h"
#include "modbusprotocol.h"

class ModbusCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusCommandWidget(QWidget* parent = nullptr);

    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void populateTable();
    void applyFilter(const QString& text);
    void updateParamInputs();

    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QTableWidget* m_table;
    QTextEdit* m_resultDisplay;

    // Parameter inputs (right panel)
    QSpinBox* m_unitIdSpin;
    QWidget* m_paramPanel;
    QLineEdit* m_addrEdit;
    QLineEdit* m_qtyEdit;
    QLineEdit* m_writeAddrEdit;
    QLineEdit* m_writeQtyEdit;
    QLineEdit* m_valueEdit;

    QVector<ModbusCommandDef> m_commands;
};

#endif // MODBUSCOMMANDWIDGET_H
```

- [ ] **Step 2: Create modbuscommandwidget.cpp**

```cpp
#include "modbuscommandwidget.h"
#include "modbusframebuilder.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QFormLayout>
#include <QRegularExpression>

// Category colors
static QMap<QString, QString> categoryColors()
{
    return {
        {"基本读写", "#4CAF50"},
        {"诊断状态", "#FF9800"},
        {"文件记录", "#9C27B0"},
        {"其他",     "#607D8B"},
    };
}

ModbusCommandWidget::ModbusCommandWidget(QWidget* parent)
    : QWidget(parent)
    , m_commands(ModbusProtocol::allCommandsRaw())
{
    setupUI();
    populateTable();
}

void ModbusCommandWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // ===== Left: command table =====
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Filter bar
    auto* filterBar = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("搜索功能码/名称...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ModbusCommandWidget::applyFilter);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem("全部类别");
    QSet<QString> categories;
    for (const auto& c : m_commands) categories.insert(c.category);
    for (const auto& cat : categories) m_categoryCombo->addItem(cat);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { applyFilter(m_searchEdit->text()); });

    filterBar->addWidget(m_searchEdit);
    filterBar->addWidget(m_categoryCombo);
    leftLayout->addLayout(filterBar);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"类型", "功能码", "名称"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row) {
        if (row < 0 || row >= m_commands.size()) return;
        // Highlight selected row
        for (int c = 0; c < m_table->columnCount(); ++c)
            m_table->item(row, c)->setSelected(true);
    });

    leftLayout->addWidget(m_table);
    mainLayout->addWidget(leftPanel, 2);

    // ===== Right: parameter panel + result =====
    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Unit ID
    auto* unitBar = new QHBoxLayout;
    unitBar->addWidget(new QLabel("Unit ID:"));
    m_unitIdSpin = new QSpinBox;
    m_unitIdSpin->setRange(1, 247);
    m_unitIdSpin->setValue(1);
    unitBar->addWidget(m_unitIdSpin);
    unitBar->addStretch();
    rightLayout->addLayout(unitBar);

    // Dynamic parameter panel
    m_paramPanel = new QWidget;
    auto* paramLayout = new QFormLayout(m_paramPanel);
    paramLayout->setContentsMargins(0, 0, 0, 0);

    m_addrEdit = new QLineEdit;
    m_addrEdit->setPlaceholderText("0x0000");
    m_qtyEdit = new QLineEdit;
    m_qtyEdit->setPlaceholderText("1");
    m_writeAddrEdit = new QLineEdit;
    m_writeAddrEdit->setPlaceholderText("0x0000");
    m_writeQtyEdit = new QLineEdit;
    m_writeQtyEdit->setPlaceholderText("1");
    m_valueEdit = new QLineEdit;
    m_valueEdit->setPlaceholderText("hex values, e.g. 00 0A 01 72");

    paramLayout->addRow("起始地址:", m_addrEdit);
    paramLayout->addRow("数量:", m_qtyEdit);
    paramLayout->addRow("写地址:", m_writeAddrEdit);
    paramLayout->addRow("写数量:", m_writeQtyEdit);
    paramLayout->addRow("值:", m_valueEdit);

    rightLayout->addWidget(m_paramPanel);

    // Send button
    auto* sendBtn = new QPushButton("发送");
    sendBtn->setFixedHeight(32);
    connect(sendBtn, &QPushButton::clicked, this, [this]() {
        int row = m_table->currentRow();
        if (row < 0) return;

        // Find actual command index (table may be filtered)
        int cmdIdx = -1;
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (i == row) {
                // Get the command code from the table item
                auto* item = m_table->item(i, 1);
                if (!item) return;
                QString codeStr = item->text();
                // Find matching command
                for (int j = 0; j < m_commands.size(); ++j) {
                    if (QString("0x%1").arg(m_commands[j].cmdCode, 2, 16, QChar('0')).toUpper() == codeStr) {
                        cmdIdx = j;
                        break;
                    }
                }
                break;
            }
        }
        if (cmdIdx < 0) return;

        const auto& cmd = m_commands[cmdIdx];
        quint8 funcCode = cmd.cmdCode;
        quint8 unitId = static_cast<quint8>(m_unitIdSpin->value());

        QByteArray params;
        params.append(static_cast<char>(unitId));

        // Build PDU data based on function code
        auto parseHex = [](const QString& text) -> QByteArray {
            QString clean = text;
            clean.remove(' ');
            clean.remove(',');
            clean.remove("0x", Qt::CaseInsensitive);
            return QByteArray::fromHex(clean.toUtf8());
        };

        auto parseAddr = [this]() -> quint16 {
            QString text = m_addrEdit->text().trimmed();
            bool ok;
            if (text.startsWith("0x", Qt::CaseInsensitive))
                return static_cast<quint16>(text.toUShort(&ok, 16));
            return static_cast<quint16>(text.toUShort(&ok, 10));
        };

        auto parseQty = [this]() -> quint16 {
            QString text = m_qtyEdit->text().trimmed();
            bool ok;
            return static_cast<quint16>(text.toUShort(&ok, text.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10));
        };

        switch (funcCode) {
        case 0x01: case 0x02: case 0x03: case 0x04: {
            // Read: address + quantity
            params.append(ModbusFrameBuilder::encodeAddress(parseAddr()));
            params.append(ModbusFrameBuilder::encodeQuantity(parseQty()));
            break;
        }
        case 0x05: {
            // Write Single Coil: address + value (FF00 or 0000)
            params.append(ModbusFrameBuilder::encodeAddress(parseAddr()));
            quint16 val = (m_valueEdit->text().trimmed().toLower() == "on" ||
                          m_valueEdit->text().trimmed() == "1") ? 0xFF00 : 0x0000;
            params.append(ModbusFrameBuilder::encodeValue(val));
            break;
        }
        case 0x06: {
            // Write Single Register: address + value
            params.append(ModbusFrameBuilder::encodeAddress(parseAddr()));
            auto hexData = parseHex(m_valueEdit->text());
            if (hexData.size() >= 2) {
                params.append(hexData.left(2));
            } else {
                bool ok;
                quint16 val = static_cast<quint16>(m_valueEdit->text().toUShort(&ok, 0));
                params.append(ModbusFrameBuilder::encodeValue(val));
            }
            break;
        }
        case 0x0F: {
            // Write Multiple Coils: address + quantity + byteCount + values
            quint16 addr = parseAddr();
            quint16 qty = parseQty();
            auto hexData = parseHex(m_valueEdit->text());
            params.append(ModbusFrameBuilder::encodeAddress(addr));
            params.append(ModbusFrameBuilder::encodeQuantity(qty));
            params.append(static_cast<char>(hexData.size()));
            params.append(hexData);
            break;
        }
        case 0x10: {
            // Write Multiple Registers: address + quantity + byteCount + values
            quint16 addr = parseAddr();
            auto hexData = parseHex(m_valueEdit->text());
            quint16 qty = static_cast<quint16>(hexData.size() / 2);
            params.append(ModbusFrameBuilder::encodeAddress(addr));
            params.append(ModbusFrameBuilder::encodeQuantity(qty));
            params.append(static_cast<char>(hexData.size()));
            params.append(hexData);
            break;
        }
        case 0x17: {
            // Read/Write Multiple Registers
            quint16 readAddr = parseAddr();
            quint16 readQty = parseQty();
            auto writeAddrText = m_writeAddrEdit->text().trimmed();
            quint16 writeAddr = 0;
            if (!writeAddrText.isEmpty()) {
                bool ok;
                writeAddr = static_cast<quint16>(writeAddrText.toUShort(&ok, writeAddrText.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10));
            }
            auto hexData = parseHex(m_valueEdit->text());
            quint16 writeQty = static_cast<quint16>(hexData.size() / 2);
            params.append(ModbusFrameBuilder::encodeAddress(readAddr));
            params.append(ModbusFrameBuilder::encodeQuantity(readQty));
            params.append(ModbusFrameBuilder::encodeAddress(writeAddr));
            params.append(ModbusFrameBuilder::encodeQuantity(writeQty));
            params.append(static_cast<char>(hexData.size()));
            params.append(hexData);
            break;
        }
        case 0x08: {
            // Diagnostics: sub-function + data
            auto hexData = parseHex(m_valueEdit->text());
            if (hexData.size() >= 2)
                params.append(hexData);
            break;
        }
        case 0x2B: {
            // MEI: type + data
            auto hexData = parseHex(m_valueEdit->text());
            params.append(hexData);
            break;
        }
        default:
            // No additional params needed (0x07, 0x0B, 0x0C, 0x11, 0x14, 0x15)
            // For 0x14/0x15, try to append hex data if provided
            if (!m_valueEdit->text().trimmed().isEmpty()) {
                params.append(parseHex(m_valueEdit->text()));
            }
            break;
        }

        emit sendCommand(funcCode, params);
    });
    rightLayout->addWidget(sendBtn);

    // Result display
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");

    rightLayout->addWidget(m_resultDisplay, 1);

    mainLayout->addWidget(rightPanel, 3);

    // Show/hide param fields on table row change
    connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row) {
        if (row < 0) return;
        // Find the command for this row
        auto* codeItem = m_table->item(row, 1);
        if (!codeItem) return;
        QString codeStr = codeItem->text();
        for (const auto& cmd : m_commands) {
            if (QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper() == codeStr) {
                // Update visibility of param fields
                bool needsAddr = (cmd.cmdCode != 0x07 && cmd.cmdCode != 0x0B &&
                                  cmd.cmdCode != 0x0C && cmd.cmdCode != 0x11);
                bool needsQty = (cmd.cmdCode == 0x01 || cmd.cmdCode == 0x02 ||
                                 cmd.cmdCode == 0x03 || cmd.cmdCode == 0x04 ||
                                 cmd.cmdCode == 0x0F || cmd.cmdCode == 0x10);
                bool needsWriteAddr = (cmd.cmdCode == 0x17);
                bool needsValue = (cmd.cmdCode == 0x05 || cmd.cmdCode == 0x06 ||
                                   cmd.cmdCode == 0x0F || cmd.cmdCode == 0x10 ||
                                   cmd.cmdCode == 0x17 || cmd.cmdCode == 0x08 ||
                                   cmd.cmdCode == 0x2B || cmd.cmdCode == 0x14 ||
                                   cmd.cmdCode == 0x15);

                m_addrEdit->setVisible(needsAddr);
                m_qtyEdit->setVisible(needsQty);
                m_writeAddrEdit->setVisible(needsWriteAddr);
                m_writeQtyEdit->setVisible(false);
                m_valueEdit->setVisible(needsValue);

                // Update form labels
                auto* formLayout = qobject_cast<QFormLayout*>(m_paramPanel->layout());
                if (formLayout) {
                    // Show/hide label+widget rows by accessing the label item
                    for (int i = 0; i < formLayout->rowCount(); ++i) {
                        auto* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
                        auto* fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
                        if (labelItem && labelItem->widget()) {
                            bool visible = false;
                            QString text = labelItem->widget()->objectName();
                            if (labelItem->widget() == formLayout->labelForField(m_addrEdit))
                                visible = needsAddr;
                            else if (labelItem->widget() == formLayout->labelForField(m_qtyEdit))
                                visible = needsQty;
                            else if (labelItem->widget() == formLayout->labelForField(m_writeAddrEdit))
                                visible = needsWriteAddr;
                            else if (labelItem->widget() == formLayout->labelForField(m_writeQtyEdit))
                                visible = false;
                            else if (labelItem->widget() == formLayout->labelForField(m_valueEdit))
                                visible = needsValue;
                            labelItem->widget()->setVisible(visible);
                        }
                    }
                }
                break;
            }
        }
    });
}

void ModbusCommandWidget::populateTable()
{
    auto colors = categoryColors();
    m_table->setRowCount(static_cast<int>(m_commands.size()));

    for (int i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];

        // Type label with color
        auto* typeLabel = new QLabel(cmd.category);
        QString color = colors.value(cmd.category, "#9E9E9E");
        typeLabel->setStyleSheet(QString("color: white; background: %1; "
                                         "border-radius: 3px; padding: 2px 6px; "
                                         "font-size: 11px;").arg(color));
        typeLabel->setAlignment(Qt::AlignCenter);
        m_table->setCellWidget(i, 0, typeLabel);

        // Function code
        auto* codeItem = new QTableWidgetItem(QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper());
        codeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, codeItem);

        // Name
        m_table->setItem(i, 2, new QTableWidgetItem(cmd.name));
    }

    // Select first row
    if (m_table->rowCount() > 0)
        m_table->selectRow(0);
}

void ModbusCommandWidget::applyFilter(const QString& text)
{
    QString category = m_categoryCombo->currentText();
    if (category == "全部类别") category.clear();

    for (int i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];

        bool matchCategory = category.isEmpty() || cmd.category == category;
        bool matchText = text.isEmpty() ||
                         cmd.name.contains(text, Qt::CaseInsensitive) ||
                         QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).contains(text, Qt::CaseInsensitive);

        m_table->setRowHidden(i, !(matchCategory && matchText));
    }
}

void ModbusCommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    auto cmd = ModbusProtocol::commandDefRaw(resp.cmdCode);

    QString html;
    html += QString("<b>[%1] 0x%2</b><br>").arg(cmd.name).arg(resp.cmdCode, 2, 16, QChar('0')).toUpper();

    if (ModbusFrameBuilder::isException(resp.cmdCode) && !resp.rawData.isEmpty()) {
        quint8 excCode = static_cast<quint8>(resp.rawData[0]);
        QString excMsg;
        switch (excCode) {
        case 1: excMsg = "非法功能码"; break;
        case 2: excMsg = "非法数据地址"; break;
        case 3: excMsg = "非法数据值"; break;
        case 4: excMsg = "从站故障"; break;
        case 5: excMsg = "确认"; break;
        case 6: excMsg = "从站忙"; break;
        case 7: excMsg = "负确认"; break;
        case 8: excMsg = "内存奇偶错误"; break;
        default: excMsg = QString("未知(%1)").arg(excCode); break;
        }
        html += QString("<span style='color:red;'>异常: %1 (0x%2)</span><br>")
                    .arg(excMsg).arg(excCode, 2, 16, QChar('0')).toUpper();
    } else if (resp.isValid) {
        html += QString("<span style='color:green;'>成功</span><br>");
    }

    html += QString("<br>%1").arg(interpretation);

    m_resultDisplay->setHtml(html);
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/modbus/modbuscommandwidget.h src/protocol/modbus/modbuscommandwidget.cpp
git commit -m "feat(modbus): add ModbusCommandWidget with function code filtering and dynamic params"
```

---

## Task 5: ModbusRealtimeWidget

**Files:**
- Create: `src/protocol/modbus/modbusrealtimewidget.h`
- Create: `src/protocol/modbus/modbusrealtimewidget.cpp`

- [ ] **Step 1: Create modbusrealtimewidget.h**

```cpp
#ifndef MODBUSREALTIMEWIDGET_H
#define MODBUSREALTIMEWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVector>
#include "protocol/iprotocol.h"

class ModbusRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusRealtimeWidget(QWidget* parent = nullptr);

    void updateData(const ParsedResponse& resp);
    void startPolling();
    void stopPolling();
    void clearData();

public slots:
    void appendHexDisplay(const QByteArray& data, bool isSend);

signals:
    void pollRequest(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void addPollItem();
    void removeSelectedPollItem();
    void startCycle();

    enum DataType { UINT16, INT16, UINT32, INT32, FLOAT32, BOOL };

    struct PollItem {
        quint16 startAddr;
        quint8 funcCode;
        quint16 quantity;
        QString name;
        DataType dataType;
    };

    // Controls
    QSpinBox* m_addrSpin;
    QComboBox* m_funcCombo;
    QSpinBox* m_qtySpin;
    QLineEdit* m_nameEdit;
    QComboBox* m_dataTypeCombo;
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;

    QCheckBox* m_autoRefreshCheck;
    QComboBox* m_intervalCombo;
    QPushButton* m_manualRefreshBtn;

    // Poll list
    QTableWidget* m_pollTable;

    // Results
    QTableWidget* m_resultTable;

    // Hex display
    QTextEdit* m_hexDisplay;

    // Polling state
    QTimer* m_cycleDelayTimer;
    int m_pollIndex;
    bool m_cycleActive;
    QVector<PollItem> m_pollItems;
};

#endif // MODBUSREALTIMEWIDGET_H
```

- [ ] **Step 2: Create modbusrealtimewidget.cpp**

```cpp
#include "modbusrealtimewidget.h"
#include "modbusframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <cstring>

ModbusRealtimeWidget::ModbusRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_cycleActive(false)
{
    setupUI();

    m_cycleDelayTimer = new QTimer(this);
    m_cycleDelayTimer->setSingleShot(true);
    connect(m_cycleDelayTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRefreshCheck->isChecked())
            startCycle();
    });

    connect(m_manualRefreshBtn, &QPushButton::clicked, this, [this]() {
        startCycle();
    });

    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_intervalCombo->setEnabled(checked);
        if (!checked)
            m_cycleDelayTimer->stop();
    });
}

void ModbusRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // === Poll configuration ===
    auto* configGroup = new QGroupBox("轮询配置");
    auto* configLayout = new QVBoxLayout(configGroup);

    // Add poll item row
    auto* addRow = new QHBoxLayout;
    addRow->addWidget(new QLabel("地址:"));
    m_addrSpin = new QSpinBox;
    m_addrSpin->setRange(0, 65535);
    m_addrSpin->setDisplayIntegerBase(16);
    m_addrSpin->setPrefix("0x");
    addRow->addWidget(m_addrSpin);

    addRow->addWidget(new QLabel("功能码:"));
    m_funcCombo = new QComboBox;
    m_funcCombo->addItem("03 - Read Holding Registers", 0x03);
    m_funcCombo->addItem("04 - Read Input Registers", 0x04);
    m_funcCombo->addItem("01 - Read Coils", 0x01);
    m_funcCombo->addItem("02 - Read Discrete Inputs", 0x02);
    addRow->addWidget(m_funcCombo);

    addRow->addWidget(new QLabel("数量:"));
    m_qtySpin = new QSpinBox;
    m_qtySpin->setRange(1, 125);
    m_qtySpin->setValue(10);
    addRow->addWidget(m_qtySpin);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("名称(可选)");
    m_nameEdit->setMaximumWidth(120);
    addRow->addWidget(m_nameEdit);

    m_dataTypeCombo = new QComboBox;
    m_dataTypeCombo->addItem("UINT16", static_cast<int>(UINT16));
    m_dataTypeCombo->addItem("INT16", static_cast<int>(INT16));
    m_dataTypeCombo->addItem("UINT32", static_cast<int>(UINT32));
    m_dataTypeCombo->addItem("INT32", static_cast<int>(INT32));
    m_dataTypeCombo->addItem("FLOAT32", static_cast<int>(FLOAT32));
    m_dataTypeCombo->addItem("BOOL", static_cast<int>(BOOL));
    addRow->addWidget(m_dataTypeCombo);

    m_addBtn = new QPushButton("+添加");
    m_addBtn->setFixedWidth(60);
    connect(m_addBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::addPollItem);
    addRow->addWidget(m_addBtn);

    m_removeBtn = new QPushButton("-删除");
    m_removeBtn->setFixedWidth(60);
    connect(m_removeBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::removeSelectedPollItem);
    addRow->addWidget(m_removeBtn);

    configLayout->addLayout(addRow);

    // Poll items table
    m_pollTable = new QTableWidget;
    m_pollTable->setColumnCount(5);
    m_pollTable->setHorizontalHeaderLabels({"地址", "功能码", "数量", "名称", "数据类型"});
    m_pollTable->horizontalHeader()->setStretchLastSection(true);
    m_pollTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pollTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pollTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pollTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_pollTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_pollTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pollTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pollTable->setMaximumHeight(120);
    configLayout->addWidget(m_pollTable);

    // Refresh controls
    auto* refreshBar = new QHBoxLayout;
    m_autoRefreshCheck = new QCheckBox("自动刷新");
    m_autoRefreshCheck->setChecked(true);
    refreshBar->addWidget(m_autoRefreshCheck);

    refreshBar->addWidget(new QLabel("间隔:"));
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItem("1 秒", 1);
    m_intervalCombo->addItem("2 秒", 2);
    m_intervalCombo->addItem("5 秒", 5);
    m_intervalCombo->addItem("10 秒", 10);
    m_intervalCombo->addItem("30 秒", 30);
    m_intervalCombo->setCurrentIndex(0);
    refreshBar->addWidget(m_intervalCombo);

    m_manualRefreshBtn = new QPushButton("手动刷新");
    m_manualRefreshBtn->setFixedWidth(80);
    refreshBar->addWidget(m_manualRefreshBtn);
    refreshBar->addStretch();

    configLayout->addLayout(refreshBar);
    mainLayout->addWidget(configGroup);

    // === Results table ===
    auto* resultGroup = new QGroupBox("轮询结果");
    auto* resultLayout = new QVBoxLayout(resultGroup);

    m_resultTable = new QTableWidget;
    m_resultTable->setColumnCount(4);
    m_resultTable->setHorizontalHeaderLabels({"地址", "类型", "原始值", "解析值"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultLayout->addWidget(m_resultTable);

    mainLayout->addWidget(resultGroup, 1);

    // === Hex display ===
    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(80);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");
    mainLayout->addWidget(m_hexDisplay);
}

void ModbusRealtimeWidget::addPollItem()
{
    PollItem item;
    item.startAddr = static_cast<quint16>(m_addrSpin->value());
    item.funcCode = static_cast<quint8>(m_funcCombo->currentData().toInt());
    item.quantity = static_cast<quint16>(m_qtySpin->value());
    item.name = m_nameEdit->text().isEmpty()
        ? QString("Reg_%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()
        : m_nameEdit->text();
    item.dataType = static_cast<DataType>(m_dataTypeCombo->currentData().toInt());

    m_pollItems.append(item);

    int row = m_pollTable->rowCount();
    m_pollTable->insertRow(row);
    m_pollTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
    m_pollTable->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(item.funcCode, 2, 16, QChar('0')).toUpper()));
    m_pollTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.quantity)));
    m_pollTable->setItem(row, 3, new QTableWidgetItem(item.name));
    QStringList typeNames = {"UINT16", "INT16", "UINT32", "INT32", "FLOAT32", "BOOL"};
    m_pollTable->setItem(row, 4, new QTableWidgetItem(typeNames[item.dataType]));

    m_pollTable->selectRow(row);
}

void ModbusRealtimeWidget::removeSelectedPollItem()
{
    int row = m_pollTable->currentRow();
    if (row < 0) return;
    m_pollTable->removeRow(row);
    m_pollItems.remove(row);
}

void ModbusRealtimeWidget::startCycle()
{
    if (m_cycleActive || m_pollItems.isEmpty()) return;
    m_cycleActive = true;
    m_pollIndex = 0;

    const auto& item = m_pollItems[0];
    QByteArray params;
    params.append('\x01'); // unitId = 1
    params.append(ModbusFrameBuilder::encodeAddress(item.startAddr));
    params.append(ModbusFrameBuilder::encodeQuantity(item.quantity));
    emit pollRequest(item.funcCode, params);
}

void ModbusRealtimeWidget::updateData(const ParsedResponse& resp)
{
    // Advance cycle
    if (m_cycleActive) {
        m_pollIndex++;
        if (m_pollIndex >= m_pollItems.size()) {
            m_cycleActive = false;
            if (m_autoRefreshCheck->isChecked()) {
                int secs = m_intervalCombo->currentData().toInt();
                m_cycleDelayTimer->start(secs * 1000);
            }
        } else {
            const auto& item = m_pollItems[m_pollIndex];
            QByteArray params;
            params.append('\x01'); // unitId = 1
            params.append(ModbusFrameBuilder::encodeAddress(item.startAddr));
            params.append(ModbusFrameBuilder::encodeQuantity(item.quantity));
            emit pollRequest(item.funcCode, params);
        }
    }

    // Update results table
    if (m_pollItems.isEmpty() || m_pollIndex == 0) return;
    int idx = m_pollIndex - 1;
    if (idx < 0 || idx >= m_pollItems.size()) return;
    const auto& item = m_pollItems[idx];

    // Ensure result table has enough rows
    while (m_resultTable->rowCount() < idx + 1)
        m_resultTable->insertRow(m_resultTable->rowCount());

    int row = idx;
    QStringList typeNames = {"UINT16", "INT16", "UINT32", "INT32", "FLOAT32", "BOOL"};

    if (!resp.isValid) {
        m_resultTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
        m_resultTable->setItem(row, 1, new QTableWidgetItem(typeNames[item.dataType]));
        m_resultTable->setItem(row, 2, new QTableWidgetItem("--"));
        m_resultTable->setItem(row, 3, new QTableWidgetItem("<span style='color:red;'>错误</span>"));
        return;
    }

    // Parse response data
    if (resp.rawData.size() < 1) return;
    int byteCount = static_cast<quint8>(resp.rawData[0]);

    // Show hex of raw data
    QStringList hexVals;
    for (int i = 1; i <= byteCount && i < resp.rawData.size(); ++i)
        hexVals << QString("%1").arg(static_cast<quint8>(resp.rawData[i]), 2, 16, QChar('0')).toUpper();

    // Decode values
    QStringList decodedVals;
    int dataOffset = 1;

    switch (item.dataType) {
    case UINT16: {
        auto regs = ModbusFrameBuilder::decodeRegisters(resp.rawData, dataOffset, item.quantity);
        for (quint16 v : regs) decodedVals << QString::number(v);
        break;
    }
    case INT16: {
        auto regs = ModbusFrameBuilder::decodeRegisters(resp.rawData, dataOffset, item.quantity);
        for (quint16 v : regs) decodedVals << QString::number(static_cast<qint16>(v));
        break;
    }
    case UINT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            quint32 v = ModbusFrameBuilder::decodeUInt32(resp.rawData, dataOffset + i * 4);
            decodedVals << QString::number(v);
        }
        break;
    }
    case INT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            qint32 v = static_cast<qint32>(ModbusFrameBuilder::decodeUInt32(resp.rawData, dataOffset + i * 4));
            decodedVals << QString::number(v);
        }
        break;
    }
    case FLOAT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            float f;
            quint32 raw = ModbusFrameBuilder::decodeUInt32(resp.rawData, dataOffset + i * 4);
            std::memcpy(&f, &raw, 4);
            decodedVals << QString::number(f, 'f', 2);
        }
        break;
    }
    case BOOL: {
        auto coils = ModbusFrameBuilder::decodeCoils(resp.rawData, dataOffset, item.quantity);
        for (bool b : coils) decodedVals << (b ? "1" : "0");
        break;
    }
    }

    m_resultTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(typeNames[item.dataType]));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(hexVals.join(" ")));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(decodedVals.join(", ")));
}

void ModbusRealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();

    QString prefix = isSend ? "[TX]" : "[RX]";
    m_hexDisplay->append(QString("%1 %2").arg(prefix, hex.join(" ")));
}

void ModbusRealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    m_resultTable->setRowCount(0);
    startCycle();
}

void ModbusRealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_cycleActive = false;
}

void ModbusRealtimeWidget::clearData()
{
    m_resultTable->setRowCount(0);
    m_hexDisplay->clear();
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/modbus/modbusrealtimewidget.h src/protocol/modbus/modbusrealtimewidget.cpp
git commit -m "feat(modbus): add ModbusRealtimeWidget with configurable polling and multi-datatype support"
```

---

## Task 6: ModbusWidgetFactory + MainWindow Integration + .pro

**Files:**
- Create: `src/protocol/modbus/modbuswidgetfactory.h`
- Create: `src/protocol/modbus/modbuswidgetfactory.cpp`
- Modify: `src/mainwindow.h`
- Modify: `src/mainwindow.cpp`
- Modify: `GSK988Tool.pro`

- [ ] **Step 1: Create modbuswidgetfactory.h**

```cpp
#ifndef MODBUSWIDGETFACTORY_H
#define MODBUSWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class ModbusWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // MODBUSWIDGETFACTORY_H
```

- [ ] **Step 2: Create modbuswidgetfactory.cpp**

```cpp
#include "modbuswidgetfactory.h"
#include "modbusrealtimewidget.h"
#include "modbuscommandwidget.h"
#include "modbusparsewidget.h"

QWidget* ModbusWidgetFactory::createRealtimeWidget()
{
    return new ModbusRealtimeWidget;
}

QWidget* ModbusWidgetFactory::createCommandWidget()
{
    return new ModbusCommandWidget;
}

QWidget* ModbusWidgetFactory::createParseWidget()
{
    return new ModbusParseWidget;
}
```

- [ ] **Step 3: Commit widget factory**

```bash
git add src/protocol/modbus/modbuswidgetfactory.h src/protocol/modbus/modbuswidgetfactory.cpp
git commit -m "feat(modbus): add ModbusWidgetFactory"
```

- [ ] **Step 4: Update mainwindow.h — add forward declarations**

Add after `class Gsk988CommandWidget;` (line 19):

```cpp
class ModbusRealtimeWidget;
class ModbusCommandWidget;
```

- [ ] **Step 5: Update mainwindow.cpp — add includes**

Add after line 7 (`#include "protocol/gsk988/gsk988commandwidget.h"`):

```cpp
#include "protocol/modbus/modbusprotocol.h"
#include "protocol/modbus/modbuswidgetfactory.h"
#include "protocol/modbus/modbusrealtimewidget.h"
#include "protocol/modbus/modbuscommandwidget.h"
```

- [ ] **Step 6: Update mainwindow.cpp setupToolBar — add "Modbus TCP" to protocol combo**

In `setupToolBar()`, after `m_protocolCombo->addItem("GSK988");` (line 64), add:

```cpp
    m_protocolCombo->addItem("Modbus TCP");
```

- [ ] **Step 7: Update mainwindow.cpp switchProtocol — add Modbus case**

Replace the hardcoded GSK988-only block (lines 146-147):

```cpp
    m_protocol = new Gsk988Protocol(this);
    m_widgetFactory = new Gsk988WidgetFactory;
```

With:

```cpp
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
```

- [ ] **Step 8: Update mainwindow.cpp switchTransport — remove GSK988-specific permission request**

In the `ITransport::connected` lambda (around line 206-213), the GSK988-specific permission request (0x0A) should only fire for GSK988 protocol. Replace:

```cpp
        // Send initial permission request (GSK988-specific: 0x0A)
        // Polling starts after receiving the 0x0A response
        m_needStartPolling = true;
        m_buffer.clear();
        QByteArray permFrame = m_protocol->buildRequest(0x0A);
        m_transport->send(permFrame);

        auto cmd = m_protocol->commandDef(0x0A);
        m_logTab->logFrame(permFrame, true, QString("[0x0A] %1").arg(cmd.name));
```

With:

```cpp
        m_buffer.clear();

        if (m_protocolCombo->currentIndex() == 0) {
            // GSK988: send initial permission request (0x0A)
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
```

- [ ] **Step 9: Update mainwindow.cpp disconnected lambda — add ModbusRealtimeWidget cast**

In the `ITransport::disconnected` lambda (around line 224-228), add Modbus handling after the Gsk988 block:

```cpp
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
```

- [ ] **Step 10: Update mainwindow.cpp setupProtocolConnections — add Modbus signal wiring**

In `setupProtocolConnections()`, add Modbus-specific wiring after the GSK988 block (after line 286). The signal names are the same (`pollRequest` and `sendCommand`), so we can use `qobject_cast`:

After the GSK988 `rt` block (after line 266), add:

```cpp
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
```

After the GSK988 `cmd` block (after line 286), add:

```cpp
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
```

- [ ] **Step 11: Update mainwindow.cpp onDataReceived — add Modbus routing**

In `onDataReceived()`, after the GSK988 realtime routing block (after line 319), add:

```cpp
        // Route to Modbus realtime tab
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) {
            modbusRt->appendHexDisplay(frame, false);
            modbusRt->updateData(resp);
        }
```

After the `m_needStartPolling` block (after line 325), the Modbus polling start is already handled in the connected signal, so no change needed there.

After the GSK988 command routing block (after line 335), add:

```cpp
            auto* modbusCmdW = qobject_cast<ModbusCommandWidget*>(m_commandTab);
            if (modbusCmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                modbusCmdW->showResponse(resp, interp);
            }
```

- [ ] **Step 12: Update mainwindow.cpp onConnectClicked — add Modbus disconnect handler**

In `onConnectClicked()`, after the GSK988 disconnect block (around line 357), add:

```cpp
        auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
        if (modbusRt) modbusRt->stopPolling();
```

- [ ] **Step 13: Update mainwindow.cpp onResponseTimeout — add Modbus timeout handler**

In `onResponseTimeout()`, after the GSK988 block (around line 349), add:

```cpp
    auto* modbusRt = qobject_cast<ModbusRealtimeWidget*>(m_realtimeTab);
    if (modbusRt) modbusRt->updateData(dummy);
```

- [ ] **Step 14: Update GSK988Tool.pro — add Modbus source files**

Add to SOURCES:

```
    src/protocol/modbus/modbusprotocol.cpp \
    src/protocol/modbus/modbusframebuilder.cpp \
    src/protocol/modbus/modbuswidgetfactory.cpp \
    src/protocol/modbus/modbusrealtimewidget.cpp \
    src/protocol/modbus/modbuscommandwidget.cpp \
    src/protocol/modbus/modbusparsewidget.cpp \
```

Add to HEADERS:

```
    src/protocol/modbus/modbusprotocol.h \
    src/protocol/modbus/modbusframebuilder.h \
    src/protocol/modbus/modbuswidgetfactory.h \
    src/protocol/modbus/modbusrealtimewidget.h \
    src/protocol/modbus/modbuscommandwidget.h \
    src/protocol/modbus/modbusparsewidget.h \
```

- [ ] **Step 15: Build and verify**

```bash
cd build_msvc && qmake ../GSK988Tool.pro && nmake
```

Expected: clean build, no errors.

- [ ] **Step 16: Commit integration**

```bash
git add src/mainwindow.h src/mainwindow.cpp GSK988Tool.pro
git commit -m "feat(modbus): integrate Modbus TCP protocol into MainWindow with protocol switching"
```
