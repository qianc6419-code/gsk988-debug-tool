#include "fanucprotocol.h"
#include "fanucframebuilder.h"

// ========== Command Definitions ==========

static QVector<FanucCommandDef> buildCommandTable()
{
    QVector<FanucCommandDef> cmds;

    auto add = [&](quint8 code, const QString& name, const QString& cat,
                   const QString& reqP, const QString& respD) {
        cmds.append({code, name, cat, reqP, respD});
    };

    // System
    add(0x01, QStringLiteral("系统信息"),       QStringLiteral("系统信息"), QStringLiteral("无"),       QStringLiteral("NC型号+设备类型"));
    add(0x02, QStringLiteral("运行信息"),       QStringLiteral("运行状态"), QStringLiteral("无"),       QStringLiteral("模式+状态+急停+告警"));

    // Spindle
    add(0x03, QStringLiteral("主轴速度"),       QStringLiteral("主轴"),     QStringLiteral("无"),       QStringLiteral("主轴速度值"));
    add(0x04, QStringLiteral("主轴负载"),       QStringLiteral("主轴"),     QStringLiteral("无"),       QStringLiteral("主轴负载百分比"));
    add(0x05, QStringLiteral("主轴倍率"),       QStringLiteral("主轴"),     QStringLiteral("无"),       QStringLiteral("主轴倍率值"));
    add(0x06, QStringLiteral("主轴速度设定值"), QStringLiteral("主轴"),     QStringLiteral("无"),       QStringLiteral("主轴速度设定值"));

    // Feed
    add(0x07, QStringLiteral("进给速度"),       QStringLiteral("进给"),     QStringLiteral("无"),       QStringLiteral("进给速度值"));
    add(0x08, QStringLiteral("进给倍率"),       QStringLiteral("进给"),     QStringLiteral("无"),       QStringLiteral("进给倍率值"));
    add(0x09, QStringLiteral("进给速度设定值"), QStringLiteral("进给"),     QStringLiteral("无"),       QStringLiteral("进给速度设定值"));

    // Counters
    add(0x0A, QStringLiteral("工件数"),         QStringLiteral("计数"),     QStringLiteral("无"),       QStringLiteral("工件数"));

    // Time
    add(0x0B, QStringLiteral("运行时间"),       QStringLiteral("时间"),     QStringLiteral("无"),       QStringLiteral("运行时间(秒)"));
    add(0x0C, QStringLiteral("加工时间"),       QStringLiteral("时间"),     QStringLiteral("无"),       QStringLiteral("加工时间(秒)"));
    add(0x0D, QStringLiteral("循环时间"),       QStringLiteral("时间"),     QStringLiteral("无"),       QStringLiteral("循环时间(秒)"));
    add(0x0E, QStringLiteral("上电时间"),       QStringLiteral("时间"),     QStringLiteral("无"),       QStringLiteral("上电时间(秒)"));

    // Program
    add(0x0F, QStringLiteral("程序号"),         QStringLiteral("程序"),     QStringLiteral("无"),       QStringLiteral("程序号"));
    add(0x10, QStringLiteral("刀具号"),         QStringLiteral("程序"),     QStringLiteral("无"),       QStringLiteral("刀具号"));

    // Alarm
    add(0x11, QStringLiteral("告警信息"),       QStringLiteral("告警"),     QStringLiteral("无"),       QStringLiteral("告警列表"));

    // Coordinates
    add(0x12, QStringLiteral("绝对坐标"),       QStringLiteral("坐标"),     QStringLiteral("无"),       QStringLiteral("XYZ坐标值"));
    add(0x13, QStringLiteral("机械坐标"),       QStringLiteral("坐标"),     QStringLiteral("无"),       QStringLiteral("XYZ坐标值"));
    add(0x14, QStringLiteral("相对坐标"),       QStringLiteral("坐标"),     QStringLiteral("无"),       QStringLiteral("XYZ坐标值"));
    add(0x15, QStringLiteral("剩余距离坐标"),   QStringLiteral("坐标"),     QStringLiteral("无"),       QStringLiteral("XYZ坐标值"));

    // Macro variables
    add(0x16, QStringLiteral("读宏变量"),       QStringLiteral("宏变量"),   QStringLiteral("地址"),     QStringLiteral("宏变量值"));
    add(0x17, QStringLiteral("写宏变量"),       QStringLiteral("宏变量"),   QStringLiteral("地址+值"),  QStringLiteral("写入结果"));

    // PMC
    add(0x18, QStringLiteral("读PMC"),          QStringLiteral("PMC"),      QStringLiteral("地址类型+地址+数据类型"), QStringLiteral("PMC值"));
    add(0x19, QStringLiteral("写PMC"),          QStringLiteral("PMC"),      QStringLiteral("地址类型+地址+值+数据类型"), QStringLiteral("写入结果"));

    // Parameters
    add(0x1A, QStringLiteral("读参数"),         QStringLiteral("参数"),     QStringLiteral("参数号"),   QStringLiteral("参数值"));

    return cmds;
}

QVector<FanucCommandDef> FanucProtocol::allCommandsRaw()
{
    static QVector<FanucCommandDef> s_commands = buildCommandTable();
    return s_commands;
}

FanucCommandDef FanucProtocol::commandDefRaw(quint8 cmdCode)
{
    auto cmds = allCommandsRaw();
    for (const auto& c : cmds) {
        if (c.cmdCode == cmdCode)
            return c;
    }
    return {cmdCode, QStringLiteral("未知命令"), QStringLiteral("其他"), QString(), QString()};
}

// ========== IProtocol: allCommands / commandDef ==========

QVector<ProtocolCommand> FanucProtocol::allCommands() const
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

ProtocolCommand FanucProtocol::commandDef(quint8 cmdCode) const
{
    FanucCommandDef raw = commandDefRaw(cmdCode);
    ProtocolCommand pc;
    pc.cmdCode = raw.cmdCode;
    pc.name = raw.name;
    pc.category = raw.category;
    pc.paramDesc = raw.requestParams;
    pc.respDesc = raw.responseDesc;
    return pc;
}

// ========== Constructor ==========

FanucProtocol::FanucProtocol(QObject* parent)
    : IProtocol(parent)
    , m_handshakeDone(false)
    , m_lastRequestCmdCode(0)
{
}

// ========== Handshake ==========

QByteArray FanucProtocol::buildHandshake()
{
    m_handshakeDone = true;
    m_lastRequestCmdCode = 0xFF; // special code for handshake
    return FanucFrameBuilder::buildHandshake();
}

// ========== Build Request ==========

QByteArray FanucProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    using FB = FanucFrameBuilder;
    m_lastRequestCmdCode = cmdCode;

    // Helper lambda: wrap a single pre-built block into a complete read frame
    auto wrapBlock = [](const QByteArray& block) -> QByteArray {
        QByteArray frame;
        frame.append(4, static_cast<char>(FB::HEADER_BYTE));
        frame.append(FB::FUNC_READ);
        frame.append(FB::encodeInt16(static_cast<qint16>(2 + block.size())));
        frame.append(FB::encodeInt16(1));
        frame.append(block);
        return frame;
    };

    switch (cmdCode) {
    // --- NC direct commands (single block, ncFlag=1) ---
    case 0x01: // 系统信息
        return FB::buildSingleBlockRequest(1, 0x18, QByteArray(20, '\0'));
    case 0x02: // 运行信息
        return FB::buildSingleBlockRequest(1, 0x19, QByteArray(20, '\0'));
    case 0x03: // 主轴速度
        return FB::buildSingleBlockRequest(1, 0x25, QByteArray(20, '\0'));
    case 0x04: { // 主轴负载
        QByteArray p;
        p.append(FB::encodeInt32(4));     // spindle number = 4 (all spindles)
        p.append(FB::encodeInt32(-1));    // info type = -1
        p.append(QByteArray(12, '\0'));
        return FB::buildSingleBlockRequest(1, 0x40, p);
    }
    case 0x05: { // 主轴倍率 - PMC read G30
        QByteArray block = FB::buildReadPMCBlock('G', 30, 30, FB::pmcAddrTypeToNum('G'), 0);
        return wrapBlock(block);
    }
    case 0x06: { // 主轴速度设定值 - PMC read F22-F25 (4 bytes, long type)
        QByteArray block = FB::buildReadPMCBlock('F', 22, 25, FB::pmcAddrTypeToNum('F'), 2);
        return wrapBlock(block);
    }
    case 0x07: // 进给速度
        return FB::buildSingleBlockRequest(1, 0x24, QByteArray(20, '\0'));
    case 0x08: { // 进给倍率 - PMC read G12
        QByteArray block = FB::buildReadPMCBlock('G', 12, 12, FB::pmcAddrTypeToNum('G'), 0);
        return wrapBlock(block);
    }
    case 0x09: { // 进给速度设定值 - macro read 4109
        QByteArray block = FB::buildReadMacroBlock(4109);
        return wrapBlock(block);
    }
    case 0x0A: { // 工件数 - macro read 3901
        QByteArray block = FB::buildReadMacroBlock(3901);
        return wrapBlock(block);
    }
    case 0x0B: { // 运行时间 - param 6751+6752
        return FB::buildMultiBlockRequest({FB::buildReadParamBlock(6751),
                                           FB::buildReadParamBlock(6752)});
    }
    case 0x0C: { // 加工时间 - param 6753+6754
        return FB::buildMultiBlockRequest({FB::buildReadParamBlock(6753),
                                           FB::buildReadParamBlock(6754)});
    }
    case 0x0D: { // 循环时间 - param 6757+6758
        return FB::buildMultiBlockRequest({FB::buildReadParamBlock(6757),
                                           FB::buildReadParamBlock(6758)});
    }
    case 0x0E: { // 上电时间 - param 6750
        QByteArray block = FB::buildReadParamBlock(6750);
        return wrapBlock(block);
    }
    case 0x0F: // 程序号
        return FB::buildSingleBlockRequest(1, 0x1C, QByteArray(20, '\0'));
    case 0x10: { // 刀具号 - macro read 4120
        QByteArray block = FB::buildReadMacroBlock(4120);
        return wrapBlock(block);
    }
    case 0x11: { // 告警信息
        QByteArray p;
        p.append(FB::encodeInt32(-1));   // info type = -1
        p.append(FB::encodeInt32(9));    // data type = 9
        p.append(FB::encodeInt32(1));    // alarm type
        p.append(FB::encodeInt32(32));   // max count
        p.append(QByteArray(4, '\0'));
        return FB::buildSingleBlockRequest(1, 0x23, p);
    }
    case 0x12: // 绝对坐标 (posType=0)
    case 0x13: // 机械坐标 (posType=1)
    case 0x14: // 相对坐标 (posType=2)
    case 0x15: { // 剩余距离坐标 (posType=3)
        quint8 posType = static_cast<quint8>(cmdCode - 0x12);
        QByteArray block = FB::buildPositionBlock(posType);
        return wrapBlock(block);
    }
    case 0x16: { // 读宏变量 - params: addr(4B big-endian)
        if (params.size() >= 4) {
            qint32 addr = FB::decodeInt32(params, 0);
            QByteArray block = FB::buildReadMacroBlock(addr);
            return wrapBlock(block);
        }
        break;
    }
    case 0x17: { // 写宏变量 - params: addr(4B) + value(4B numerator) + precision(2B)
        if (params.size() >= 10) {
            qint32 addr = FB::decodeInt32(params, 0);
            qint32 numerator = FB::decodeInt32(params, 4);
            qint16 precision = FB::decodeInt16(params, 8);
            QByteArray block = FB::buildWriteMacroBlock(addr, numerator,
                                                         static_cast<quint8>(precision));
            QByteArray frame;
            frame.append(4, static_cast<char>(FB::HEADER_BYTE));
            frame.append(FB::FUNC_READ);
            frame.append(FB::encodeInt16(static_cast<qint16>(2 + block.size())));
            frame.append(FB::encodeInt16(1));
            frame.append(block);
            return frame;
        }
        break;
    }
    case 0x18: { // 读PMC - params: addrType(1B) + startAddr(4B) + endAddr(4B) + dataType(4B)
        if (params.size() >= 13) {
            char addrType = params[0];
            int startAddr = FB::decodeInt32(params, 1);
            int endAddr = FB::decodeInt32(params, 5);
            int dataType = FB::decodeInt32(params, 9);
            QByteArray block = FB::buildReadPMCBlock(addrType, startAddr, endAddr,
                                                      FB::pmcAddrTypeToNum(addrType), dataType);
            return wrapBlock(block);
        }
        break;
    }
    case 0x19: { // 写PMC - params: addrType(1B) + startAddr(4B) + endAddr(4B) + dataType(4B) + value
        if (params.size() >= 14) {
            char addrType = params[0];
            int startAddr = FB::decodeInt32(params, 1);
            int endAddr = FB::decodeInt32(params, 5);
            int dataType = FB::decodeInt32(params, 9);
            QByteArray block;
            switch (dataType) {
            case 0: { // byte
                quint8 val = static_cast<quint8>(params[13]);
                block = FB::buildWritePMCBlockByte(addrType, startAddr, endAddr,
                                                    FB::pmcAddrTypeToNum(addrType), val);
                break;
            }
            case 1: { // word
                qint16 val = FB::decodeInt16(params, 13);
                block = FB::buildWritePMCBlockWord(addrType, startAddr, endAddr,
                                                    FB::pmcAddrTypeToNum(addrType), val);
                break;
            }
            case 2: { // long
                qint32 val = FB::decodeInt32(params, 13);
                block = FB::buildWritePMCBlockLong(addrType, startAddr, endAddr,
                                                    FB::pmcAddrTypeToNum(addrType), val);
                break;
            }
            default:
                return QByteArray();
            }
            QByteArray frame;
            frame.append(4, static_cast<char>(FB::HEADER_BYTE));
            frame.append(FB::FUNC_READ);
            frame.append(FB::encodeInt16(static_cast<qint16>(2 + block.size())));
            frame.append(FB::encodeInt16(1));
            frame.append(block);
            return frame;
        }
        break;
    }
    case 0x1A: { // 读参数 - params: paramNo(4B big-endian)
        if (params.size() >= 4) {
            qint32 paramNo = FB::decodeInt32(params, 0);
            QByteArray block = FB::buildReadParamBlock(paramNo);
            return wrapBlock(block);
        }
        break;
    }
    }

    return QByteArray();
}

// ========== Extract Frame ==========

QByteArray FanucProtocol::extractFrame(QByteArray& buffer)
{
    return FanucFrameBuilder::extractFrame(buffer);
}

// ========== Parse Response ==========

ParsedResponse FanucProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse result;
    result.cmdCode = 0;
    result.isValid = false;

    if (frame.size() < 10)
        return result;

    if (!FanucFrameBuilder::validateHeader(frame))
        return result;

    QByteArray funcCode = frame.mid(4, 4);
    using FB = FanucFrameBuilder;

    // Response function codes (seen by client receiving mock/real device data)
    static const QByteArray FUNC_HANDSHAKE_RESP("\x00\x01\x01\x02", 4);
    bool isResponse = (funcCode == FB::FUNC_RESPONSE) ||
                      (funcCode == FUNC_HANDSHAKE_RESP);

    // Request function codes (seen by mock server receiving client requests)
    static const QByteArray FUNC_HANDSHAKE_REQ("\x00\x01\x01\x01", 4);
    bool isRequest = (funcCode == FB::FUNC_READ) ||
                     (funcCode == FUNC_HANDSHAKE_REQ);

    if (!isResponse && !isRequest)
        return result;

    result.isValid = true;
    result.rawData = frame.mid(10);

    // For request frames (processed by mock server), provide the stored cmdCode
    if (isRequest) {
        result.cmdCode = m_lastRequestCmdCode;
    }
    // For response frames, cmdCode stays 0 (FanucRealtimeWidget tracks via m_pollIndex)

    return result;
}

// ========== Interpret Data ==========

QString FanucProtocol::interpretDataRaw(quint8 cmdCode, const QByteArray& data)
{
    using FB = FanucFrameBuilder;

    auto hexDump = [](const QByteArray& ba) -> QString {
        QStringList hex;
        for (int i = 0; i < ba.size(); ++i)
            hex << QString("%1").arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return hex.join(" ");
    };

    if (data.isEmpty())
        return QStringLiteral("(无数据)");

    switch (cmdCode) {
    case 0x01: { // 系统信息
        if (data.size() < 34) return hexDump(data);
        // Response block: 28B header + data
        // NC type string at block offset 8..27 (20 bytes), device type at block offset 28..31
        QString ncType = QString::fromLatin1(data.mid(8, 20)).trimmed();
        qint32 devType = FB::decodeInt32(data, 28);
        return QStringLiteral("NC型号: %1, 设备类型: %2").arg(ncType).arg(devType);
    }
    case 0x02: { // 运行信息
        if (data.size() < 38) return hexDump(data);
        // Block header 28B, then: mode(2B), status(2B), emergency(2B), alarm(2B)
        qint16 mode = FB::decodeInt16(data, 28);
        qint16 status = FB::decodeInt16(data, 30);
        qint16 emg = FB::decodeInt16(data, 32);
        qint16 alm = FB::decodeInt16(data, 34);
        QString modeStr;
        switch (mode) {
        case 0: modeStr = QStringLiteral("MDI"); break;
        case 1: modeStr = QStringLiteral("MEM"); break;
        case 3: modeStr = QStringLiteral("NO SEARCH"); break;
        case 4: modeStr = QStringLiteral("EDIT"); break;
        case 5: modeStr = QStringLiteral("HANDLE"); break;
        case 7: modeStr = QStringLiteral("INC"); break;
        case 8: modeStr = QStringLiteral("JOG"); break;
        case 11: modeStr = QStringLiteral("REF"); break;
        default: modeStr = QString::number(mode); break;
        }
        return QStringLiteral("模式: %1, 状态: %2, 急停: %3, 告警: %4")
                   .arg(modeStr).arg(status).arg(emg).arg(alm);
    }
    case 0x03: // 主轴速度
    case 0x07: { // 进给速度
        if (data.size() < 36) return hexDump(data);
        double val = FB::calcValue(data, 28);
        return QStringLiteral("%1: %2").arg(cmdCode == 0x03 ? "主轴速度" : "进给速度")
                                       .arg(val, 0, 'f', 1);
    }
    case 0x04: { // 主轴负载
        if (data.size() < 40) return hexDump(data);
        qint32 count = FB::decodeInt32(data, 28);
        if (count < 1) return hexDump(data);
        qint32 loadVal = FB::decodeInt32(data, 32);
        return QStringLiteral("主轴负载: %1%").arg(loadVal);
    }
    case 0x05: { // 主轴倍率
        if (data.size() < 32) return hexDump(data);
        quint8 val = static_cast<quint8>(data[28]);
        return QStringLiteral("主轴倍率: %1%").arg(val);
    }
    case 0x06: { // 主轴速度设定值
        if (data.size() < 32) return hexDump(data);
        qint32 val = FB::decodeInt32(data, 28);
        return QStringLiteral("主轴速度设定值: %1").arg(val);
    }
    case 0x08: { // 进给倍率
        if (data.size() < 32) return hexDump(data);
        quint8 val = static_cast<quint8>(data[28]);
        return QStringLiteral("进给倍率: %1%").arg(val);
    }
    case 0x09: { // 进给速度设定值 (macro variable)
        if (data.size() < 40) return hexDump(data);
        double val = FB::calcValue(data, 28);
        return QStringLiteral("进给速度设定值: %1").arg(val, 0, 'f', 1);
    }
    case 0x0A: { // 工件数 (macro variable)
        if (data.size() < 40) return hexDump(data);
        double val = FB::calcValue(data, 28);
        return QStringLiteral("工件数: %1").arg(val, 0, 'f', 0);
    }
    case 0x0B: // 运行时间 (param 6751+6752)
    case 0x0C: // 加工时间 (param 6753+6754)
    case 0x0D: // 循环时间 (param 6757+6758)
    case 0x0E: { // 上电时间 (param 6750)
        if (data.size() < 36) return hexDump(data);
        qint32 val = FB::decodeInt32(data, 28);
        int seconds = val;
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        QString label;
        switch (cmdCode) {
        case 0x0B: label = QStringLiteral("运行时间"); break;
        case 0x0C: label = QStringLiteral("加工时间"); break;
        case 0x0D: label = QStringLiteral("循环时间"); break;
        case 0x0E: label = QStringLiteral("上电时间"); break;
        default: label = QStringLiteral("时间"); break;
        }
        return QStringLiteral("%1: %2:%3:%4 (%5秒)")
                   .arg(label)
                   .arg(hours, 2, 10, QChar('0'))
                   .arg(mins, 2, 10, QChar('0'))
                   .arg(secs, 2, 10, QChar('0'))
                   .arg(seconds);
    }
    case 0x0F: { // 程序号
        if (data.size() < 36) return hexDump(data);
        qint32 progNo = FB::decodeInt32(data, 28);
        return QStringLiteral("程序号: O%1").arg(progNo);
    }
    case 0x10: { // 刀具号 (macro variable)
        if (data.size() < 40) return hexDump(data);
        double val = FB::calcValue(data, 28);
        return QStringLiteral("刀具号: T%1").arg(val, 0, 'f', 0);
    }
    case 0x11: { // 告警信息
        if (data.size() < 32) return hexDump(data);
        qint32 almCount = FB::decodeInt32(data, 28);
        if (almCount <= 0)
            return QStringLiteral("告警: 无");
        QStringList alarms;
        int offset = 32;
        for (qint32 i = 0; i < almCount && offset + 8 <= data.size(); ++i) {
            qint32 almNo = FB::decodeInt32(data, offset);
            qint32 almType = FB::decodeInt32(data, offset + 4);
            alarms << QStringLiteral("No.%1(Type:%2)").arg(almNo).arg(almType);
            offset += 8;
        }
        return QStringLiteral("告警(%1条): %2").arg(almCount).arg(alarms.join(", "));
    }
    case 0x12: // 绝对坐标
    case 0x13: // 机械坐标
    case 0x14: // 相对坐标
    case 0x15: { // 剩余距离坐标
        if (data.size() < 36) return hexDump(data);
        qint32 axisCount = FB::decodeInt32(data, 28);
        QStringList coords;
        int offset = 32;
        static const char axisNames[] = {'X', 'Y', 'Z', 'A', 'B', 'C', 'U', 'V', 'W'};
        for (qint32 i = 0; i < axisCount && offset + 8 <= data.size(); ++i) {
            double val = FB::calcValue(data, offset);
            char axis = (i < 9) ? axisNames[i] : ('0' + static_cast<char>(i));
            coords << QStringLiteral("%1=%2").arg(axis).arg(val, 0, 'f', 3);
            offset += 8;
        }
        QString label;
        switch (cmdCode) {
        case 0x12: label = QStringLiteral("绝对坐标"); break;
        case 0x13: label = QStringLiteral("机械坐标"); break;
        case 0x14: label = QStringLiteral("相对坐标"); break;
        case 0x15: label = QStringLiteral("剩余距离坐标"); break;
        default: label = QStringLiteral("坐标"); break;
        }
        return QStringLiteral("%1(%2轴): %3").arg(label).arg(axisCount).arg(coords.join(", "));
    }
    case 0x16: { // 读宏变量
        if (data.size() < 40) return hexDump(data);
        double val = FB::calcValue(data, 28);
        return QStringLiteral("宏变量值: %1").arg(val, 0, 'f', 4);
    }
    case 0x17: { // 写宏变量
        if (data.size() < 40) return hexDump(data);
        double val = FB::calcValue(data, 28);
        return QStringLiteral("写入结果: %1").arg(val, 0, 'f', 4);
    }
    case 0x18: { // 读PMC
        if (data.size() < 32) return hexDump(data);
        // Response data after block header
        QByteArray pmcData = data.mid(28);
        if (pmcData.size() == 1)
            return QStringLiteral("PMC值: %1 (0x%2)")
                       .arg(static_cast<quint8>(pmcData[0]))
                       .arg(static_cast<quint8>(pmcData[0]), 2, 16, QChar('0')).toUpper();
        if (pmcData.size() == 2) {
            qint16 val = FB::decodeInt16(pmcData, 0);
            return QStringLiteral("PMC值: %1 (0x%2)")
                       .arg(val)
                       .arg(static_cast<quint16>(val), 4, 16, QChar('0')).toUpper();
        }
        if (pmcData.size() == 4) {
            qint32 val = FB::decodeInt32(pmcData, 0);
            return QStringLiteral("PMC值: %1 (0x%2)")
                       .arg(val)
                       .arg(val, 8, 16, QChar('0')).toUpper();
        }
        return QStringLiteral("PMC数据: %1").arg(hexDump(pmcData));
    }
    case 0x19: { // 写PMC
        if (data.size() < 32) return hexDump(data);
        return QStringLiteral("PMC写入成功");
    }
    case 0x1A: { // 读参数
        if (data.size() < 40) return hexDump(data);
        qint32 val = FB::decodeInt32(data, 28);
        return QStringLiteral("参数值: %1 (0x%2)")
                   .arg(val)
                   .arg(val, 8, 16, QChar('0')).toUpper();
    }
    default:
        return QStringLiteral("HEX: %1").arg(hexDump(data));
    }
}

QString FanucProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    return interpretDataRaw(cmdCode, data);
}

// ========== Mock Response Data (internal payload) ==========

QByteArray FanucProtocol::mockResponseData(quint8 cmdCode, const QByteArray& requestData)
{
    using FB = FanucFrameBuilder;

    switch (cmdCode) {
    case 0x01: { // 系统信息 - block response with NC type + device type
        QByteArray block(QByteArray(28, '\0'));
        // Block length
        block[0] = '\x00'; block[1] = '\x1C';
        // NC type at offset 8..27
        QByteArray ncType = QByteArray("31i            ", 20); // "31i" padded to 20
        block.replace(8, 20, ncType);
        // Device type at offset 28..31 (but block is only 28B so append)
        block.append(FB::encodeInt32(0x0B));
        // Pad block length field
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x02: { // 运行信息
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24'; // length 36
        // mode=1(MEM), status=1(running), emg=0, alm=0
        block.append(FB::encodeInt16(1));   // mode
        block.append(FB::encodeInt16(1));   // status
        block.append(FB::encodeInt16(0));   // emergency
        block.append(FB::encodeInt16(0));   // alarm
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x03: { // 主轴速度 = 1500.0
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24'; // length 36
        // calcValue encoding: numerator / base^exp
        // 1500.0 = 15000000 / 10^4
        block.append(FB::encodeInt32(15000000));
        block.append(FB::encodeInt16(10));   // base
        block.append(FB::encodeInt16(4));    // exp
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x04: { // 主轴负载 = 45%
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24'; // length 36
        block.append(FB::encodeInt32(1));   // count = 1
        block.append(FB::encodeInt32(45));  // load = 45%
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x05: { // 主轴倍率 = 100%
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x1D'; // length 29
        block.append(static_cast<char>(100));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x06: { // 主轴速度设定值 = 2000
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20'; // length 32
        block.append(FB::encodeInt32(2000));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x07: { // 进给速度 = 500.0
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24'; // length 36
        // 500.0 = 5000000 / 10^4
        block.append(FB::encodeInt32(5000000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x08: { // 进给倍率 = 120%
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x1D'; // length 29
        block.append(static_cast<char>(120));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x09: { // 进给速度设定值 = 800.0 (macro 4109)
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24'; // length 36
        // 800.0 = 8000000 / 10^4
        block.append(FB::encodeInt32(8000000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x0A: { // 工件数 = 42 (macro 3901)
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24'; // length 36
        // 42 = 420000 / 10^4
        block.append(FB::encodeInt32(420000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x0B: { // 运行时间 = 3600秒 (param 6751)
        QByteArray block1(QByteArray(28, '\0'));
        block1[0] = '\x00'; block1[1] = '\x20'; // length 32
        block1.append(FB::encodeInt32(3600));
        qint32 len1 = block1.size();
        block1[0] = static_cast<char>((len1 >> 8) & 0xFF);
        block1[1] = static_cast<char>(len1 & 0xFF);
        return block1;
    }
    case 0x0C: { // 加工时间 = 1800秒 (param 6753)
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20';
        block.append(FB::encodeInt32(1800));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x0D: { // 循环时间 = 120秒 (param 6757)
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20';
        block.append(FB::encodeInt32(120));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x0E: { // 上电时间 = 86400秒 (param 6750)
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20';
        block.append(FB::encodeInt32(86400));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x0F: { // 程序号 = O1234
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20';
        block.append(FB::encodeInt32(1234));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x10: { // 刀具号 = T5 (macro 4120)
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24';
        // 5 = 50000 / 10^4
        block.append(FB::encodeInt32(50000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x11: { // 告警信息 - no alarms
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20';
        block.append(FB::encodeInt32(0)); // count = 0
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x12: // 绝对坐标
    case 0x13: // 机械坐标
    case 0x14: // 相对坐标
    case 0x15: { // 剩余距离坐标 - 3 axes: X=100.500, Y=200.300, Z=-50.000
        QByteArray block(QByteArray(28, '\0'));
        // axis count = 3
        block.append(FB::encodeInt32(3));
        // X = 100.500 = 1005000 / 10^4
        block.append(FB::encodeInt32(1005000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        // Y = 200.300 = 2003000 / 10^4
        block.append(FB::encodeInt32(2003000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        // Z = -50.000 = -500000 / 10^4
        block.append(FB::encodeInt32(-500000));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x16: { // 读宏变量 - return value from request addr
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24';
        // Return mock value 123.4560 = 1234560 / 10^4
        block.append(FB::encodeInt32(1234560));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x17: { // 写宏变量 - echo back the written value
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x24';
        // Echo the numerator from the request if available, else default
        qint32 numerator = 0;
        if (requestData.size() >= 8) {
            // Request data contains the write macro block; numerator is at the end
            // For simplicity, return mock 100.0
            numerator = 1000000;
        }
        block.append(FB::encodeInt32(numerator));
        block.append(FB::encodeInt16(10));
        block.append(FB::encodeInt16(4));
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x18: { // 读PMC - return 1 byte value
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x1D'; // length 29
        block.append(static_cast<char>(0x01)); // mock PMC value
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    case 0x19: { // 写PMC - return success
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x1C'; // length 28
        return block;
    }
    case 0x1A: { // 读参数 - return param value
        QByteArray block(QByteArray(28, '\0'));
        block[0] = '\x00'; block[1] = '\x20';
        block.append(FB::encodeInt32(100)); // mock param value
        qint32 totalLen = block.size();
        block[0] = static_cast<char>((totalLen >> 8) & 0xFF);
        block[1] = static_cast<char>(totalLen & 0xFF);
        return block;
    }
    default:
        return QByteArray();
    }
}

// ========== Mock Response (IProtocol - returns complete frame) ==========

QByteArray FanucProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    // Special: handshake response (cmdCode 0xFF)
    if (cmdCode == 0xFF) {
        QByteArray frame;
        frame.append(4, static_cast<char>(FanucFrameBuilder::HEADER_BYTE));
        frame.append('\x00');
        frame.append('\x01');
        frame.append('\x01');
        frame.append('\x02'); // response
        frame.append('\x00');
        frame.append('\x02');
        frame.append('\x00');
        frame.append('\x02');
        return frame;
    }

    QByteArray payload = mockResponseData(cmdCode, requestData);
    if (payload.isEmpty())
        return QByteArray();

    return FanucFrameBuilder::buildResponse(payload);
}
