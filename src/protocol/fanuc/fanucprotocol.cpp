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
    case 0x0A: { // 工件数 - param read 6711 (标准FOCAS方法)
        QByteArray block = FB::buildReadParamBlock(6711);
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
    case 0x10: { // 刀具号 - macro read 4320 (更广泛正确的选择)
        QByteArray block = FB::buildReadMacroBlock(4320);
        return wrapBlock(block);
    }
    case 0x11: { // 告警信息
        QByteArray p;
        p.append(FB::encodeInt32(-1));   // info type = -1 (all types)
        p.append(FB::encodeInt32(9));    // data type = 9 (alarm count)
        p.append(FB::encodeInt32(1));    // start position = 1
        p.append(FB::encodeInt32(32));   // alarm content length = 32
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

    // Frame bytes 4-5 are sequence number (varies, don't check).
    // Frame bytes 6-7 are the actual function code:
    //   0x21 0x01 = data request,  0x21 0x02 = data response
    //   0x01 0x01 = handshake req,  0x01 0x02 = handshake resp
    // Reference implementation only checks bytes 6-7.
    if (frame.size() < 8)
        return result;

    quint8 b6 = static_cast<quint8>(frame[6]);
    quint8 b7 = static_cast<quint8>(frame[7]);

    bool isResponse = (b6 == 0x21 && b7 == 0x02) ||   // data response
                      (b6 == 0x01 && b7 == 0x02);      // handshake response

    bool isRequest =  (b6 == 0x21 && b7 == 0x01) ||   // data request
                      (b6 == 0x01 && b7 == 0x01);      // handshake request

    if (!isResponse && !isRequest)
        return result;

    result.isValid = true;
    result.rawData = frame.mid(10);

    // Both request and response frames: use the stored cmdCode.
    // Fanuc FOCAS is strictly request-response, so m_lastRequestCmdCode
    // is always valid when the matching response arrives.
    result.cmdCode = m_lastRequestCmdCode;

    return result;
}

// ========== Interpret Data ==========

QString FanucProtocol::interpretDataRaw(quint8 cmdCode, const QByteArray& data)
{
    using FB = FanucFrameBuilder;

    // NOTE: rawData = frame.mid(10), so all offsets are frame_offset - 10.
    // Reference implementation uses frame-level offsets:
    //   frame[0-9]  = header (A0x4 + funcCode(4) + dataLen(2))
    //   frame[10-11] = block count
    //   frame[12-13] = block1 length
    //   frame[14-15] = block1 ncFlag
    //   frame[16-17] = block1 reserved
    //   frame[18-19] = block1 funcCode
    //   frame[20+]  = block1 params + data
    // rawData offsets = frame offsets - 10

    auto hexDump = [](const QByteArray& ba) -> QString {
        QStringList hex;
        for (int i = 0; i < ba.size(); ++i)
            hex << QString("%1").arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return hex.join(" ");
    };

    if (data.isEmpty())
        return QStringLiteral("(无数据)");

    switch (cmdCode) {
    case 0x01: { // 系统信息 — ref: ncType at frame[32] (2 chars), deviceType at frame[34] (2 chars)
        if (data.size() < 26) return hexDump(data);
        char ncType[3] = {0};
        char deviceType[3] = {0};
        memcpy(ncType, data.constData() + 22, 2);
        memcpy(deviceType, data.constData() + 24, 2);
        return QStringLiteral("NC型号: %1, 设备类型: %2").arg(QString(ncType)).arg(QString(deviceType));
    }
    case 0x02: { // 运行信息 — ref: mode at frame[28], status at frame[30], emg at frame[36], alm at frame[38]
        if (data.size() < 30) return hexDump(data);
        qint16 mode = FB::decodeInt16(data, 18);
        qint16 status = FB::decodeInt16(data, 20);
        qint16 emg = FB::decodeInt16(data, 26);
        qint16 alm = FB::decodeInt16(data, 28);
        QString modeStr;
        switch (mode) {
        case 0: modeStr = QStringLiteral("MDI"); break;
        case 1: modeStr = QStringLiteral("MEM"); break;
        case 3: modeStr = QStringLiteral("EDIT"); break;
        case 4: modeStr = QStringLiteral("HANDLE"); break;
        case 5: modeStr = QStringLiteral("JOG"); break;
        case 6: modeStr = QStringLiteral("TEACH-JOG"); break;
        case 7: modeStr = QStringLiteral("TEACH-HANDLE"); break;
        case 8: modeStr = QStringLiteral("INC"); break;
        case 9: modeStr = QStringLiteral("REF"); break;
        case 10: modeStr = QStringLiteral("RMT"); break;
        default: modeStr = QString::number(mode); break;
        }
        QString statusStr;
        switch (status) {
        case 0: statusStr = QStringLiteral("RESET"); break;
        case 1: statusStr = QStringLiteral("STOP"); break;
        case 2: statusStr = QStringLiteral("HOLD"); break;
        case 3: statusStr = QStringLiteral("START"); break;
        case 4: statusStr = QStringLiteral("MSTR"); break;
        default: statusStr = QString::number(status); break;
        }
        return QStringLiteral("模式: %1, 状态: %2, 急停: %3, 告警: %4")
                   .arg(modeStr).arg(statusStr).arg(emg).arg(alm);
    }
    case 0x03: // 主轴速度 — ref: calcValue at frame[28]
    case 0x07: { // 进给速度 — ref: calcValue at frame[28]
        if (data.size() < 26) return hexDump(data);
        double val = FB::calcValue(data, 18);
        return QStringLiteral("%1: %2").arg(cmdCode == 0x03 ? "主轴速度" : "进给速度")
                                       .arg(val, 0, 'f', 2);
    }
    case 0x04: { // 主轴负载 — ref: calcValue at frame[28]
        if (data.size() < 26) return hexDump(data);
        double val = FB::calcValue(data, 18);
        return QStringLiteral("主轴负载: %1%").arg(val, 0, 'f', 2);
    }
    case 0x05: { // 主轴倍率 — ref: data[28] as byte
        if (data.size() < 19) return hexDump(data);
        quint8 val = static_cast<quint8>(data[18]);
        return QStringLiteral("主轴倍率: %1%").arg(val);
    }
    case 0x06: { // 主轴速度设定值 — ref: getIntValue at frame[28]
        if (data.size() < 22) return hexDump(data);
        qint32 val = FB::decodeInt32(data, 18);
        return QStringLiteral("主轴速度设定值: %1").arg(val);
    }
    case 0x08: { // 进给倍率 — ref: 255 - data[28]
        if (data.size() < 19) return hexDump(data);
        int val = 255 - static_cast<quint8>(data[18]);
        return QStringLiteral("进给倍率: %1%").arg(val);
    }
    case 0x09: { // 进给速度设定值 (macro) — ref: calcValue at frame[28]
        if (data.size() < 26) return hexDump(data);
        double val = FB::calcValue(data, 18);
        return QStringLiteral("进给速度设定值: %1").arg(val, 0, 'f', 2);
    }
    case 0x0A: { // 工件数 (param 6711) — ref: calcValue at rawData[26]
        if (data.size() < 34) return hexDump(data);
        double val = FB::calcValue(data, 26);
        return QStringLiteral("工件数: %1").arg(val, 0, 'f', 0);
    }
    case 0x0B: // 运行时间 (param 6751+6752, 2 blocks) — ref: calcValue at frame[36], second at frame[36+block1Len]
    case 0x0C: // 加工时间 (param 6753+6754)
    case 0x0D: { // 循环时间 (param 6757+6758)
        if (data.size() < 34) return hexDump(data);
        // Multi-block param read: block header is 24 bytes, data at offset 26
        double ms = FB::calcValue(data, 26);
        qint16 block1Len = FB::decodeInt16(data, 2);
        double minutes = 0;
        if (data.size() >= 26 + block1Len + 8)
            minutes = FB::calcValue(data, 26 + block1Len);
        int seconds = static_cast<int>(minutes) * 60 + static_cast<int>(ms / 1000);
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        QString label;
        switch (cmdCode) {
        case 0x0B: label = QStringLiteral("运行时间"); break;
        case 0x0C: label = QStringLiteral("加工时间"); break;
        case 0x0D: label = QStringLiteral("循环时间"); break;
        default: label = QStringLiteral("时间"); break;
        }
        return QStringLiteral("%1: %2:%3:%4 (%5秒)")
                   .arg(label)
                   .arg(hours, 2, 10, QChar('0'))
                   .arg(mins, 2, 10, QChar('0'))
                   .arg(secs, 2, 10, QChar('0'))
                   .arg(seconds);
    }
    case 0x0E: { // 上电时间 (param 6750, 1 block) — ref: calcValue at frame[36], min*60
        if (data.size() < 34) return hexDump(data);
        double minutes = FB::calcValue(data, 26);
        int seconds = static_cast<int>(minutes) * 60;
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        return QStringLiteral("上电时间: %1:%2:%3 (%4秒)")
                   .arg(hours, 2, 10, QChar('0'))
                   .arg(mins, 2, 10, QChar('0'))
                   .arg(secs, 2, 10, QChar('0'))
                   .arg(seconds);
    }
    case 0x0F: { // 程序号 — ref: getIntValue at frame[28]
        if (data.size() < 22) return hexDump(data);
        qint32 progNo = FB::decodeInt32(data, 18);
        return QStringLiteral("程序号: O%1").arg(progNo);
    }
    case 0x10: { // 刀具号 (macro) — ref: calcValue at frame[28]
        if (data.size() < 26) return hexDump(data);
        double val = FB::calcValue(data, 18);
        return QStringLiteral("刀具号: T%1").arg(val, 0, 'f', 0);
    }
    case 0x11: { // 告警信息 — ref: dataLength at frame[24], num=dataLen/48, each 48B
        if (data.size() < 22) return hexDump(data);
        qint32 dataLength = FB::decodeInt32(data, 14);
        unsigned int num = static_cast<unsigned int>(dataLength) / 48;
        if (num == 0)
            return QStringLiteral("告警: 无");
        QStringList alarms;
        for (unsigned int i = 0; i < num; ++i) {
            int base = 18 + static_cast<int>(i) * 48;
            if (base + 48 > data.size()) break;
            qint32 almNo = FB::decodeInt32(data, base);
            qint32 almType = FB::decodeInt32(data, base + 4);
            qint32 almLen = FB::decodeInt32(data, base + 12);
            if (almLen > 0 && almLen <= 32) {
                QByteArray msg = data.mid(base + 16, almLen);
                alarms << QStringLiteral("No.%1 (Type:%2) %3")
                          .arg(almNo).arg(almType).arg(QString::fromLatin1(msg));
            } else {
                alarms << QStringLiteral("No.%1 (Type:%2)").arg(almNo).arg(almType);
            }
        }
        return QStringLiteral("告警(%1条): %2").arg(num).arg(alarms.join("\n"));
    }
    case 0x12: // 绝对坐标 — ref: x=calcValue(frame+28), y=calcValue(frame+36), z=calcValue(frame+44)
    case 0x13: // 机械坐标
    case 0x14: // 相对坐标
    case 0x15: { // 剩余距离坐标
        if (data.size() < 34) return hexDump(data);
        double x = FB::calcValue(data, 18);
        double y = (data.size() >= 34) ? FB::calcValue(data, 26) : 0.0;
        double z = (data.size() >= 42) ? FB::calcValue(data, 34) : 0.0;
        QString label;
        switch (cmdCode) {
        case 0x12: label = QStringLiteral("绝对坐标"); break;
        case 0x13: label = QStringLiteral("机械坐标"); break;
        case 0x14: label = QStringLiteral("相对坐标"); break;
        case 0x15: label = QStringLiteral("剩余距离坐标"); break;
        default: label = QStringLiteral("坐标"); break;
        }
        return QStringLiteral("%1: X=%2, Y=%3, Z=%4")
                   .arg(label)
                   .arg(x, 0, 'f', 3)
                   .arg(y, 0, 'f', 3)
                   .arg(z, 0, 'f', 3);
    }
    case 0x16: { // 读宏变量 — ref: calcValue at frame[28]
        if (data.size() < 26) return hexDump(data);
        double val = FB::calcValue(data, 18);
        return QStringLiteral("宏变量值: %1").arg(val, 0, 'f', 4);
    }
    case 0x17: { // 写宏变量
        if (data.size() < 26) return hexDump(data);
        return QStringLiteral("宏变量写入成功");
    }
    case 0x18: { // 读PMC — ref: data[28] for byte, or word/long/float
        if (data.size() < 19) return hexDump(data);
        QByteArray pmcData = data.mid(18);
        if (pmcData.size() == 1)
            return QStringLiteral("PMC值: %1 (0x%2)")
                       .arg(static_cast<quint8>(pmcData[0]))
                       .arg(static_cast<quint8>(pmcData[0]), 2, 16, QChar('0')).toUpper();
        if (pmcData.size() >= 2) {
            // Read as little-endian (reference swaps bytes)
            union { unsigned char b[2]; short v; } u_word;
            u_word.b[0] = static_cast<unsigned char>(pmcData[1]);
            u_word.b[1] = static_cast<unsigned char>(pmcData[0]);
            if (pmcData.size() == 2)
                return QStringLiteral("PMC值: %1").arg(u_word.v);
        }
        if (pmcData.size() >= 4) {
            union { unsigned char b[4]; long v; } u_long;
            u_long.b[0] = static_cast<unsigned char>(pmcData[3]);
            u_long.b[1] = static_cast<unsigned char>(pmcData[2]);
            u_long.b[2] = static_cast<unsigned char>(pmcData[1]);
            u_long.b[3] = static_cast<unsigned char>(pmcData[0]);
            return QStringLiteral("PMC值: %1").arg(u_long.v);
        }
        return QStringLiteral("PMC数据: %1").arg(hexDump(pmcData));
    }
    case 0x19: { // 写PMC
        if (data.size() < 19) return hexDump(data);
        return QStringLiteral("PMC写入成功");
    }
    case 0x1A: { // 读参数 — ref: calcValue at frame[36] (param block has 24-byte header)
        if (data.size() < 34) return hexDump(data);
        double val = FB::calcValue(data, 26);
        return QStringLiteral("参数值: %1").arg(val, 0, 'f', 4);
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
//
// The payload becomes rawData = frame.mid(10), so offsets must match:
//   rawData[0-1]  = block count
//   rawData[2-3]  = block 1 length
//   rawData[4-5]  = block 1 ncFlag
//   rawData[6-7]  = block 1 reserved
//   rawData[8-9]  = block 1 funcCode
//   rawData[10+]  = block 1 params + data
//
// NC direct commands: data at rawData[18] (block header 16 bytes)
// Param read commands: data at rawData[26] (block header 24 bytes)

// Helper: build a response block with 8-byte params (NC direct commands)
// Returns: block_count(2B) + block(length(2B)+ncFlag(2B)+reserved(2B)+funcCode(2B)+params(8B)+data)
static QByteArray buildMockNCBlock(quint16 funcCode, const QByteArray& data)
{
    QByteArray payload;
    payload.reserve(2 + 16 + data.size());
    // Block count
    payload.append('\x00');
    payload.append('\x01');
    // Block length = 16 header + data size
    quint16 blockLen = static_cast<quint16>(16 + data.size());
    payload.append(FanucFrameBuilder::encodeInt16(blockLen));
    // ncFlag
    payload.append('\x00');
    payload.append('\x01');
    // reserved
    payload.append('\x00');
    payload.append('\x01');
    // funcCode
    payload.append(FanucFrameBuilder::encodeInt16(funcCode));
    // 8 bytes params area
    payload.append(QByteArray(8, '\0'));
    // data
    payload.append(data);
    return payload;
}

// Helper: build a response block with 16-byte params (param read commands)
// Returns: block_count(2B) + block(length(2B)+ncFlag(2B)+reserved(2B)+funcCode(2B)+params(16B)+data)
static QByteArray buildMockParamBlock(const QByteArray& data)
{
    QByteArray payload;
    payload.reserve(2 + 24 + data.size());
    // Block count
    payload.append('\x00');
    payload.append('\x01');
    // Block length = 24 header + data size
    quint16 blockLen = static_cast<quint16>(24 + data.size());
    payload.append(FanucFrameBuilder::encodeInt16(blockLen));
    // ncFlag
    payload.append('\x00');
    payload.append('\x01');
    // reserved
    payload.append('\x00');
    payload.append('\x01');
    // funcCode = 0x008D (param read)
    payload.append('\x00');
    payload.append(static_cast<char>(0x8D));
    // 16 bytes params area
    payload.append(QByteArray(16, '\0'));
    // data
    payload.append(data);
    return payload;
}

// Helper: build multi-block param response (for time commands)
static QByteArray buildMockMultiParamBlock(const QByteArray& data1, const QByteArray& data2)
{
    QByteArray payload;
    payload.reserve(2 + 2 * (24 + 8));
    // Block count = 2
    payload.append('\x00');
    payload.append('\x02');
    // Block 1
    quint16 blockLen = static_cast<quint16>(24 + data1.size());
    payload.append(FanucFrameBuilder::encodeInt16(blockLen));
    payload.append('\x00'); payload.append('\x01'); // ncFlag
    payload.append('\x00'); payload.append('\x01'); // reserved
    payload.append('\x00'); payload.append(static_cast<char>(0x8D)); // funcCode
    payload.append(QByteArray(16, '\0')); // params
    payload.append(data1);
    // Block 2
    blockLen = static_cast<quint16>(24 + data2.size());
    payload.append(FanucFrameBuilder::encodeInt16(blockLen));
    payload.append('\x00'); payload.append('\x01'); // ncFlag
    payload.append('\x00'); payload.append('\x01'); // reserved
    payload.append('\x00'); payload.append(static_cast<char>(0x8D)); // funcCode
    payload.append(QByteArray(16, '\0')); // params
    payload.append(data2);
    return payload;
}

// Helper: encode calcValue as 8 bytes: numerator(4B) + base(2B) + exp(2B)
static QByteArray encodeCalcValue(double value)
{
    using FB = FanucFrameBuilder;
    // Convert to numerator / 10^4 format
    qint32 numerator = static_cast<qint32>(value * 10000);
    QByteArray data;
    data.append(FB::encodeInt32(numerator));
    data.append(FB::encodeInt16(10));  // base
    data.append(FB::encodeInt16(4));   // exp
    return data;
}

QByteArray FanucProtocol::mockResponseData(quint8 cmdCode, const QByteArray& requestData)
{
    using FB = FanucFrameBuilder;

    switch (cmdCode) {
    case 0x01: { // 系统信息 — ncType at rawData[22], deviceType at rawData[24]
        QByteArray payload;
        payload.reserve(30);
        // Block count
        payload.append('\x00'); payload.append('\x01');
        // Block length = 28
        payload.append('\x00'); payload.append('\x1C');
        // ncFlag
        payload.append('\x00'); payload.append('\x01');
        // reserved
        payload.append('\x00'); payload.append('\x01');
        // funcCode = 0x0018
        payload.append('\x00'); payload.append('\x18');
        // rawData[10-21]: 12 bytes padding
        payload.append(QByteArray(12, '\0'));
        // rawData[22-23]: ncType = "31"
        payload.append("31", 2);
        // rawData[24-25]: deviceType = "0B"
        payload.append("0B", 2);
        // rawData[26-27]: padding
        payload.append(QByteArray(2, '\0'));
        return payload;
    }
    case 0x02: { // 运行信息 — mode at rawData[18], status at [20], emg at [26], alm at [28]
        QByteArray data;
        data.append(FB::encodeInt16(1));   // mode = MEM
        data.append(FB::encodeInt16(1));   // status = STOP
        data.append(FB::encodeInt16(0));   // padding
        data.append(FB::encodeInt16(0));   // padding
        data.append(FB::encodeInt16(0));   // emg = 0
        data.append(FB::encodeInt16(0));   // alm = 0
        return buildMockNCBlock(0x19, data);
    }
    case 0x03: // 主轴速度 — calcValue at rawData[18]
        return buildMockNCBlock(0x25, encodeCalcValue(1500.0));
    case 0x04: // 主轴负载 — calcValue at rawData[18]
        return buildMockNCBlock(0x40, encodeCalcValue(45.0));
    case 0x05: { // 主轴倍率 — byte at rawData[18]
        QByteArray data;
        data.append(static_cast<char>(100)); // 100%
        return buildMockNCBlock(0x01, data); // funcCode echoed, value at data offset
    }
    case 0x06: { // 主轴速度设定值 — int32 at rawData[18]
        QByteArray data;
        data.append(FB::encodeInt32(2000));
        return buildMockNCBlock(0x01, data);
    }
    case 0x07: // 进给速度 — calcValue at rawData[18]
        return buildMockNCBlock(0x24, encodeCalcValue(500.0));
    case 0x08: { // 进给倍率 — 255 - byte at rawData[18]
        QByteArray data;
        data.append(static_cast<char>(255 - 120)); // 120% stored as 255-120
        return buildMockNCBlock(0x01, data);
    }
    case 0x09: // 进给速度设定值 — calcValue at rawData[18]
        return buildMockNCBlock(0x15, encodeCalcValue(800.0));
    case 0x0A: // 工件数 — param 6711: calcValue at rawData[26]
        return buildMockParamBlock(encodeCalcValue(42.0));
    case 0x0B: { // 运行时间 — multi-block: ms=calcValue(rawData[26]), min=calcValue(rawData[26+block1Len])
        // Block 1: ms (param 6751) = 30000 ms
        // Block 2: min (param 6752) = 60 minutes → 60*60 + 30000/1000 = 3630 seconds
        return buildMockMultiParamBlock(encodeCalcValue(30000.0), encodeCalcValue(60.0));
    }
    case 0x0C: { // 加工时间
        return buildMockMultiParamBlock(encodeCalcValue(18000.0), encodeCalcValue(30.0));
    }
    case 0x0D: { // 循环时间
        return buildMockMultiParamBlock(encodeCalcValue(0.0), encodeCalcValue(2.0));
    }
    case 0x0E: // 上电时间 — single param: calcValue at rawData[26], min*60
        return buildMockParamBlock(encodeCalcValue(1440.0)); // 1440 minutes = 86400 seconds
    case 0x0F: { // 程序号 — int32 at rawData[18]
        QByteArray data;
        data.append(FB::encodeInt32(1234));
        return buildMockNCBlock(0x1C, data);
    }
    case 0x10: // 刀具号 — calcValue at rawData[18]
        return buildMockNCBlock(0x15, encodeCalcValue(5.0));
    case 0x11: { // 告警信息 — dataLength at rawData[14], no alarms
        QByteArray payload;
        payload.reserve(22);
        payload.append('\x00'); payload.append('\x01'); // block count
        payload.append('\x00'); payload.append('\x10'); // block length = 16
        payload.append('\x00'); payload.append('\x01'); // ncFlag
        payload.append('\x00'); payload.append('\x01'); // reserved
        payload.append('\x00'); payload.append('\x23'); // funcCode = 0x23
        payload.append(QByteArray(4, '\0')); // rawData[10-13]
        payload.append(FB::encodeInt32(0));  // rawData[14-17] = dataLength = 0
        return payload;
    }
    case 0x12: // 绝对坐标
    case 0x13: // 机械坐标
    case 0x14: // 相对坐标
    case 0x15: { // 剩余距离坐标 — x=calcValue(rawData[18]), y=calcValue(rawData[26]), z=calcValue(rawData[34])
        QByteArray data;
        data.append(encodeCalcValue(100.500));  // X
        data.append(encodeCalcValue(200.300));  // Y
        data.append(encodeCalcValue(-50.000));  // Z
        return buildMockNCBlock(0x26, data);
    }
    case 0x16: // 读宏变量 — calcValue at rawData[18]
        return buildMockNCBlock(0x15, encodeCalcValue(123.456));
    case 0x17: // 写宏变量
        return buildMockNCBlock(0x16, QByteArray());
    case 0x18: { // 读PMC — byte at rawData[18]
        QByteArray data;
        data.append(static_cast<char>(0x01));
        return buildMockNCBlock(0x01, data); // PMC response with 1 byte
    }
    case 0x19: // 写PMC
        return buildMockNCBlock(0x02, QByteArray());
    case 0x1A: // 读参数 — calcValue at rawData[26]
        return buildMockParamBlock(encodeCalcValue(100.0));
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
