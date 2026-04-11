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
    s_mockCoils = QVector<bool>(65536, false);
    s_mockDiscreteInputs = QVector<bool>(65536, false);
    s_mockHoldingRegs = QVector<quint16>(65536, 0);
    s_mockInputRegs = QVector<quint16>(65536, 0);

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

    add(0x01, "Read Coils",                   "基本读写", "起始地址(2B)+数量(2B,1-2000)",           "字节数(1B)+线圈状态(nB)");
    add(0x02, "Read Discrete Inputs",         "基本读写", "起始地址(2B)+数量(2B,1-2000)",           "字节数(1B)+输入状态(nB)");
    add(0x03, "Read Holding Registers",       "基本读写", "起始地址(2B)+数量(2B,1-125)",            "字节数(1B)+寄存器值(nB)");
    add(0x04, "Read Input Registers",         "基本读写", "起始地址(2B)+数量(2B,1-125)",            "字节数(1B)+寄存器值(nB)");
    add(0x05, "Write Single Coil",            "基本读写", "输出地址(2B)+输出值(2B:FF00/0000)",      "回显请求");
    add(0x06, "Write Single Register",        "基本读写", "寄存器地址(2B)+寄存器值(2B)",            "回显请求");
    add(0x0F, "Write Multiple Coils",         "基本读写", "起始地址(2B)+数量(2B)+字节数(1B)+值(nB)", "起始地址(2B)+数量(2B)");
    add(0x10, "Write Multiple Registers",     "基本读写", "起始地址(2B)+数量(2B)+字节数(1B)+值(nB)", "起始地址(2B)+数量(2B)");

    add(0x07, "Read Exception Status",        "诊断状态", "无",                                    "异常状态(1B)");
    add(0x08, "Diagnostics",                  "诊断状态", "子功能(2B)+数据(nB)",                    "子功能(2B)+数据(nB)");
    add(0x0B, "Get Comm Event Counter",       "诊断状态", "无",                                    "状态(2B)+事件计数(2B)");
    add(0x0C, "Get Comm Event Log",           "诊断状态", "无",                                    "状态(2B)+事件计数(2B)+消息计数(2B)+事件(nB)");

    add(0x14, "Read File Record",             "文件记录", "字节数(1B)+引用组(n×7B)",                "文件响应数据");
    add(0x15, "Write File Record",            "文件记录", "完整引用组数据",                          "回显请求");

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

// ========== Build Request ==========

QByteArray ModbusProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
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
    if (buffer.size() < 6)
        return QByteArray();

    quint16 length = (static_cast<quint8>(buffer[4]) << 8) |
                     static_cast<quint8>(buffer[5]);

    int totalFrameSize = 6 + length;

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

    if (frame.size() < 8) {
        result.isValid = false;
        return result;
    }

    quint16 protoId = ModbusFrameBuilder::parseProtocolId(frame);
    if (protoId != 0x0000) {
        result.isValid = false;
        return result;
    }

    quint8 funcCode = ModbusFrameBuilder::parseFunctionCode(frame);
    result.cmdCode = funcCode;

    if (ModbusFrameBuilder::isException(funcCode)) {
        result.isValid = false;
        result.cmdCode = funcCode & 0x7F;
        result.rawData = frame.mid(8);
        return result;
    }

    result.isValid = true;
    result.rawData = frame.mid(8);
    return result;
}

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
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 val = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("地址: %1 (0x%2), 值: %3 (%4)")
                   .arg(addr).arg(addr, 4, 16, QChar('0')).toUpper()
                   .arg(val == 0xFF00 ? "ON" : "OFF")
                   .arg(val, 4, 16, QChar('0')).toUpper();
    }
    case 0x06: {
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 val = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("地址: %1 (0x%2), 值: %3 (0x%4)")
                   .arg(addr).arg(addr, 4, 16, QChar('0')).toUpper()
                   .arg(val).arg(val, 4, 16, QChar('0')).toUpper();
    }
    case 0x0F: {
        if (data.size() < 4) return hexDump(data);
        quint16 addr = ModbusFrameBuilder::decodeUInt16(data, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(data, 2);
        return QString("起始地址: %1, 写入数量: %2").arg(addr).arg(qty);
    }
    case 0x10: {
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

// ========== Mock Response Data (internal) ==========

QByteArray ModbusProtocol::mockResponseData(quint8 cmdCode, const QByteArray& requestData)
{
    initMockMemory();
    QByteArray respData;

    switch (cmdCode) {
    case 0x01: {
        if (requestData.size() < 4) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        if (qty < 1 || qty > 2000) break;
        int byteCount = (qty + 7) / 8;
        QByteArray coilBytes(byteCount, '\0');
        for (int i = 0; i < qty && (addr + i) < s_mockCoils.size(); ++i) {
            if (s_mockCoils[addr + i])
                coilBytes[i / 8] = static_cast<char>(static_cast<quint8>(coilBytes[i / 8]) | (1 << (i % 8)));
        }
        respData.append(static_cast<char>(byteCount));
        respData.append(coilBytes);
        break;
    }
    case 0x02: {
        if (requestData.size() < 4) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        if (qty < 1 || qty > 2000) break;
        int byteCount = (qty + 7) / 8;
        QByteArray inputBytes(byteCount, '\0');
        for (int i = 0; i < qty && (addr + i) < s_mockDiscreteInputs.size(); ++i) {
            if (s_mockDiscreteInputs[addr + i])
                inputBytes[i / 8] = static_cast<char>(static_cast<quint8>(inputBytes[i / 8]) | (1 << (i % 8)));
        }
        respData.append(static_cast<char>(byteCount));
        respData.append(inputBytes);
        break;
    }
    case 0x03: {
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
        if (requestData.size() < 5) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
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
        if (requestData.size() < 5) break;
        quint16 addr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 qty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
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
        respData.append('\x00');
        break;
    case 0x08: {
        if (requestData.size() >= 4) {
            respData.append(requestData);
        }
        break;
    }
    case 0x0B:
        respData.append(ModbusFrameBuilder::encodeValue(0x0000));
        respData.append(ModbusFrameBuilder::encodeValue(42));
        break;
    case 0x0C:
        respData.append(ModbusFrameBuilder::encodeValue(0x0000));
        respData.append(ModbusFrameBuilder::encodeValue(42));
        respData.append(ModbusFrameBuilder::encodeValue(100));
        respData.append('\x20');
        break;
    case 0x11:
        respData.append('\xFF');
        respData.append('\xFF');
        respData.append('\x00');
        break;
    case 0x14:
        respData.append('\x00');
        break;
    case 0x15:
        respData = requestData;
        break;
    case 0x17: {
        if (requestData.size() < 9) break;
        quint16 readAddr = ModbusFrameBuilder::decodeUInt16(requestData, 0);
        quint16 readQty = ModbusFrameBuilder::decodeUInt16(requestData, 2);
        quint16 writeAddr = ModbusFrameBuilder::decodeUInt16(requestData, 4);
        quint16 writeQty = ModbusFrameBuilder::decodeUInt16(requestData, 6);
        quint8 writeByteCount = static_cast<quint8>(requestData[8]);

        if (requestData.size() >= 9 + writeByteCount) {
            for (int i = 0; i < writeQty && (writeAddr + i) < s_mockHoldingRegs.size(); ++i) {
                s_mockHoldingRegs[writeAddr + i] = ModbusFrameBuilder::decodeUInt16(requestData, 9 + i * 2);
            }
        }

        int readByteCount = readQty * 2;
        respData.append(static_cast<char>(readByteCount));
        for (int i = 0; i < readQty && (readAddr + i) < s_mockHoldingRegs.size(); ++i) {
            respData.append(ModbusFrameBuilder::encodeValue(s_mockHoldingRegs[readAddr + i]));
        }
        break;
    }
    case 0x2B:
        if (requestData.size() >= 1) {
            respData.append(requestData[0]);
            respData.append('\x00');
        }
        break;
    default:
        return QByteArray();
    }

    return respData;
}

// ========== Mock Response (IProtocol — returns complete frame) ==========

QByteArray ModbusProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    QByteArray respData = mockResponseData(cmdCode, requestData);
    if (respData.isEmpty()) {
        quint16 txnId = m_transactionId;
        quint8 unitId = 1;
        QByteArray excData;
        excData.append('\x01');
        return ModbusFrameBuilder::buildFrame(txnId, unitId, cmdCode | 0x80, excData);
    }

    quint16 txnId = m_transactionId;
    quint8 unitId = 1;
    return ModbusFrameBuilder::buildFrame(txnId, unitId, cmdCode, respData);
}
