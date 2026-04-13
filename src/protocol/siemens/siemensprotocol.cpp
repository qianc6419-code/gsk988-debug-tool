#include "siemensprotocol.h"
#include "siemensframebuilder.h"
#include <cstring>

// ========== Command Definitions ==========

static QVector<SiemensCommandDef> buildCommandTable()
{
    QVector<SiemensCommandDef> cmds;

    auto add = [&](quint8 code, const QString& name, const QString& cat,
                   const QString& dtype, const QString& desc) {
        cmds.append({code, name, cat, dtype, desc});
    };

    // 系统信息
    add(0x01, QStringLiteral("CNC标识"),       QStringLiteral("系统信息"), QStringLiteral("String"),  QStringLiteral("机床指纹"));
    add(0x02, QStringLiteral("CNC型号"),       QStringLiteral("系统信息"), QStringLiteral("String"),  QStringLiteral("机床型号"));
    add(0x03, QStringLiteral("版本信息"),       QStringLiteral("系统信息"), QStringLiteral("String"),  QStringLiteral("NC版本"));
    add(0x04, QStringLiteral("制造日期"),       QStringLiteral("系统信息"), QStringLiteral("String"),  QStringLiteral("出厂日期"));

    // 运行状态
    add(0x05, QStringLiteral("操作模式"),       QStringLiteral("运行状态"), QStringLiteral("Mode"),    QStringLiteral("JOG/MDA/AUTO/REFPOINT"));
    add(0x06, QStringLiteral("运行状态"),       QStringLiteral("运行状态"), QStringLiteral("Status"),  QStringLiteral("RESET/STOP/START"));

    // 主轴
    add(0x07, QStringLiteral("主轴设定速度"),   QStringLiteral("主轴"),     QStringLiteral("Double"), QStringLiteral("RPM"));
    add(0x08, QStringLiteral("主轴实际速度"),   QStringLiteral("主轴"),     QStringLiteral("Double"), QStringLiteral("RPM"));
    add(0x09, QStringLiteral("主轴倍率"),       QStringLiteral("主轴"),     QStringLiteral("Double"), QStringLiteral("%"));

    // 进给
    add(0x0A, QStringLiteral("进给设定速度"),   QStringLiteral("进给"),     QStringLiteral("Double"), QStringLiteral("mm/min"));
    add(0x0B, QStringLiteral("进给实际速度"),   QStringLiteral("进给"),     QStringLiteral("Double"), QStringLiteral("mm/min"));
    add(0x0C, QStringLiteral("进给倍率"),       QStringLiteral("进给"),     QStringLiteral("Double"), QStringLiteral("%"));

    // 计数与时间
    add(0x0D, QStringLiteral("工件数"),         QStringLiteral("计数时间"), QStringLiteral("Double"), QStringLiteral("实际加工数"));
    add(0x0E, QStringLiteral("设定工件数"),     QStringLiteral("计数时间"), QStringLiteral("Double"), QStringLiteral("设定目标数"));
    add(0x0F, QStringLiteral("循环时间"),       QStringLiteral("计数时间"), QStringLiteral("Double"), QStringLiteral("秒"));
    add(0x10, QStringLiteral("剩余时间"),       QStringLiteral("计数时间"), QStringLiteral("Double"), QStringLiteral("秒"));

    // 程序与刀具
    add(0x11, QStringLiteral("程序名"),         QStringLiteral("程序刀具"), QStringLiteral("String"),  QStringLiteral("当前程序"));
    add(0x12, QStringLiteral("程序内容"),       QStringLiteral("程序刀具"), QStringLiteral("String"),  QStringLiteral("当前程序内容"));
    add(0x13, QStringLiteral("刀具号"),         QStringLiteral("程序刀具"), QStringLiteral("Int16"),   QStringLiteral("T号"));
    add(0x14, QStringLiteral("刀具半径D"),      QStringLiteral("程序刀具"), QStringLiteral("Int16"),   QStringLiteral("D补偿号"));
    add(0x15, QStringLiteral("刀具长度H"),      QStringLiteral("程序刀具"), QStringLiteral("Int16"),   QStringLiteral("H补偿号"));
    add(0x16, QStringLiteral("长度补偿X"),      QStringLiteral("程序刀具"), QStringLiteral("Double"), QStringLiteral("X补偿值"));
    add(0x17, QStringLiteral("长度补偿Z"),      QStringLiteral("程序刀具"), QStringLiteral("Double"), QStringLiteral("Z补偿值"));
    add(0x18, QStringLiteral("刀具磨损半径"),   QStringLiteral("程序刀具"), QStringLiteral("Double"), QStringLiteral("磨损值"));
    add(0x19, QStringLiteral("刀沿位置"),       QStringLiteral("程序刀具"), QStringLiteral("Double"), QStringLiteral("EDG"));

    // 坐标
    add(0x1A, QStringLiteral("机械坐标-X"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x01"));
    add(0x1B, QStringLiteral("机械坐标-Y"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x02"));
    add(0x1C, QStringLiteral("机械坐标-Z"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x03"));
    add(0x1D, QStringLiteral("工件坐标-X"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x01"));
    add(0x1E, QStringLiteral("工件坐标-Y"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x02"));
    add(0x1F, QStringLiteral("工件坐标-Z"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x03"));
    add(0x20, QStringLiteral("剩余坐标-X"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x01"));
    add(0x21, QStringLiteral("剩余坐标-Y"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x02"));
    add(0x22, QStringLiteral("剩余坐标-Z"),     QStringLiteral("坐标"),     QStringLiteral("Double"), QStringLiteral("posflag=0x03"));
    add(0x23, QStringLiteral("轴名称"),         QStringLiteral("坐标"),     QStringLiteral("String"),  QStringLiteral("轴名"));

    // 驱动诊断
    add(0x24, QStringLiteral("母线电压"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("V"));
    add(0x25, QStringLiteral("驱动电流"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("A"));
    add(0x26, QStringLiteral("电机功率-X"),     QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("kW"));
    add(0x27, QStringLiteral("驱动负载2"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("%"));
    add(0x28, QStringLiteral("驱动负载3"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("%"));
    add(0x29, QStringLiteral("驱动负载4"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("%"));
    add(0x2A, QStringLiteral("主轴负载1"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("%"));
    add(0x2B, QStringLiteral("主轴负载2"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("%"));
    add(0x2C, QStringLiteral("电机温度"),       QStringLiteral("驱动诊断"), QStringLiteral("Float"),   QStringLiteral("°C"));

    // 告警
    add(0x2D, QStringLiteral("告警数量"),       QStringLiteral("告警"),     QStringLiteral("Int32"),   QStringLiteral(""));
    add(0x2E, QStringLiteral("告警信息"),       QStringLiteral("告警"),     QStringLiteral("Int32"),   QStringLiteral("需告警索引"));

    // 变量读写
    add(0x2F, QStringLiteral("读R变量"),        QStringLiteral("变量读写"), QStringLiteral("Double"), QStringLiteral("需变量地址"));
    add(0x30, QStringLiteral("写R变量"),        QStringLiteral("变量读写"), QStringLiteral("Double"), QStringLiteral("需addr+值"));
    add(0x31, QStringLiteral("R驱动器"),        QStringLiteral("变量读写"), QStringLiteral("Float"),   QStringLiteral("需addr+axis+index"));

    // 工件坐标系
    add(0x32, QStringLiteral("T工件坐标系"), QStringLiteral("工件坐标系"), QStringLiteral("Int16"), QStringLiteral("T坐标系"));
    add(0x33, QStringLiteral("M工件坐标系"), QStringLiteral("工件坐标系"), QStringLiteral("Int16"), QStringLiteral("M坐标系"));

    // PLC
    add(0x34, QStringLiteral("PLC告警"),     QStringLiteral("PLC"),       QStringLiteral("String"),  QStringLiteral("DB1600, 需偏移参数"));

    return cmds;
}

QVector<SiemensCommandDef> SiemensProtocol::allCommandsRaw()
{
    static QVector<SiemensCommandDef> s_commands = buildCommandTable();
    return s_commands;
}

SiemensCommandDef SiemensProtocol::commandDefRaw(quint8 cmdCode)
{
    auto cmds = allCommandsRaw();
    for (const auto& c : cmds) {
        if (c.cmdCode == cmdCode)
            return c;
    }
    return {cmdCode, QStringLiteral("未知命令"), QStringLiteral("其他"), QString(), QString()};
}

// ========== IProtocol: allCommands / commandDef ==========

QVector<ProtocolCommand> SiemensProtocol::allCommands() const
{
    QVector<ProtocolCommand> result;
    auto rawCmds = allCommandsRaw();
    result.reserve(rawCmds.size());
    for (const auto& c : rawCmds) {
        ProtocolCommand pc;
        pc.cmdCode = c.cmdCode;
        pc.name = c.name;
        pc.category = c.category;
        pc.paramDesc = c.dataType;
        pc.respDesc = c.desc;
        result.append(pc);
    }
    return result;
}

ProtocolCommand SiemensProtocol::commandDef(quint8 cmdCode) const
{
    SiemensCommandDef raw = commandDefRaw(cmdCode);
    ProtocolCommand pc;
    pc.cmdCode = raw.cmdCode;
    pc.name = raw.name;
    pc.category = raw.category;
    pc.paramDesc = raw.dataType;
    pc.respDesc = raw.desc;
    return pc;
}

// ========== Constructor ==========

SiemensProtocol::SiemensProtocol(QObject* parent)
    : IProtocol(parent)
    , m_handshakeStep(0)
    , m_lastRequestCmdCode(0)
{
}

// ========== Handshake ==========

QByteArray SiemensProtocol::buildHandshake()
{
    m_handshakeStep = 1;
    m_lastRequestCmdCode = 0xFF;
    return SiemensFrameBuilder::HANDSHAKE_1;
}

bool SiemensProtocol::isHandshakeComplete() const
{
    return m_handshakeStep >= 4;
}

QByteArray SiemensProtocol::advanceHandshake()
{
    m_handshakeStep++;
    switch (m_handshakeStep) {
    case 2:
        m_lastRequestCmdCode = 0xFE;
        return SiemensFrameBuilder::HANDSHAKE_2;
    case 3:
        m_lastRequestCmdCode = 0xFD;
        return SiemensFrameBuilder::HANDSHAKE_3;
    default:
        return QByteArray();
    }
}

// ========== Build Request ==========

QByteArray SiemensProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    using FB = SiemensFrameBuilder;
    m_lastRequestCmdCode = cmdCode;

    switch (cmdCode) {
    // 系统信息
    case 0x01: return FB::CMD_CNC_ID;
    case 0x02: return FB::CMD_CNC_TYPE;
    case 0x03: return FB::CMD_VER_INFO;
    case 0x04: return FB::CMD_MANUF_DATE;

    // 运行状态
    case 0x05: return FB::CMD_MODE;
    case 0x06: return FB::CMD_STATUS;

    // 主轴
    case 0x07: return FB::CMD_SPINDLE_SET_SPEED;
    case 0x08: return FB::CMD_SPINDLE_ACT_SPEED;
    case 0x09: return FB::CMD_SPINDLE_RATE;

    // 进给
    case 0x0A: return FB::CMD_FEED_SET_SPEED;
    case 0x0B: return FB::CMD_FEED_ACT_SPEED;
    case 0x0C: return FB::CMD_FEED_RATE;

    // 计数与时间
    case 0x0D: return FB::CMD_PRODUCTS;
    case 0x0E: return FB::CMD_SET_PRODUCTS;
    case 0x0F: return FB::CMD_CYCLE_TIME;
    case 0x10: return FB::CMD_REMAIN_TIME;

    // 程序与刀具
    case 0x11: return FB::CMD_PROG_NAME;
    case 0x12: return FB::CMD_PROG_CONTENT;
    case 0x13: return FB::CMD_TOOL_NUM;
    case 0x14: return FB::CMD_TOOL_D;
    case 0x15: return FB::CMD_TOOL_H;
    case 0x16: return FB::CMD_TOOL_X_LEN;
    case 0x17: return FB::CMD_TOOL_Z_LEN;
    case 0x18: return FB::CMD_TOOL_RADIU;
    case 0x19: return FB::CMD_TOOL_EDG;

    // 坐标 - 机械坐标 (0x1A-0x1C)
    case 0x1A: return FB::buildPositionCmd(FB::CMD_MACH_POS, 0x01);
    case 0x1B: return FB::buildPositionCmd(FB::CMD_MACH_POS, 0x02);
    case 0x1C: return FB::buildPositionCmd(FB::CMD_MACH_POS, 0x03);

    // 坐标 - 工件坐标 (0x1D-0x1F)
    case 0x1D: return FB::buildPositionCmd(FB::CMD_WORK_POS, 0x01);
    case 0x1E: return FB::buildPositionCmd(FB::CMD_WORK_POS, 0x02);
    case 0x1F: return FB::buildPositionCmd(FB::CMD_WORK_POS, 0x03);

    // 坐标 - 剩余坐标 (0x20-0x22)
    case 0x20: return FB::buildPositionCmd(FB::CMD_REMAIN_POS, 0x01);
    case 0x21: return FB::buildPositionCmd(FB::CMD_REMAIN_POS, 0x02);
    case 0x22: return FB::buildPositionCmd(FB::CMD_REMAIN_POS, 0x03);

    case 0x23: return FB::CMD_AXIS_NAME;

    // 驱动诊断
    case 0x24: return FB::CMD_DRIVE_VOLTAGE;
    case 0x25: return FB::CMD_DRIVE_CURRENT;
    case 0x26: return FB::CMD_DRIVE_LOAD1;
    case 0x27: return FB::CMD_DRIVE_LOAD2;
    case 0x28: return FB::CMD_DRIVE_LOAD3;
    case 0x29: return FB::CMD_DRIVE_LOAD4;
    case 0x2A: return FB::CMD_SPINDLE_LOAD1;
    case 0x2B: return FB::CMD_SPINDLE_LOAD2;
    case 0x2C: return FB::CMD_DRIVE_TEMPER;

    // 告警
    case 0x2D: return FB::CMD_ALARM_NUM;
    case 0x2E: {
        if (params.size() >= 1)
            return FB::buildAlarmCmd(static_cast<int>(static_cast<quint8>(params[0])));
        return FB::buildAlarmCmd(1);
    }

    // 变量读写
    case 0x2F: {
        if (params.size() >= 4) {
            qint32 addr;
            std::memcpy(&addr, params.constData(), 4);
            return FB::buildReadRCmd(addr);
        }
        return FB::buildReadRCmd(1);
    }
    case 0x30: {
        if (params.size() >= 12) {
            qint32 addr;
            double val;
            std::memcpy(&addr, params.constData(), 4);
            std::memcpy(&val, params.constData() + 4, 8);
            return FB::buildWriteRCmd(addr, val);
        }
        return FB::buildWriteRCmd(1, 0.0);
    }
    case 0x31: {
        if (params.size() >= 3)
            return FB::buildRDriverCmd(static_cast<quint8>(params[0]),
                                        static_cast<quint8>(params[1]),
                                        static_cast<quint8>(params[2]));
        return FB::buildRDriverCmd(0xA1, 0x25, 0x01);
    }

    // 工件坐标系
    case 0x32: return FB::CMD_T_WORK_SYSTEM;
    case 0x33: return FB::CMD_M_WORK_SYSTEM;

    // PLC
    case 0x34: {
        if (params.size() >= 4) {
            qint32 offset;
            std::memcpy(&offset, params.constData(), 4);
            return FB::buildPLCReadCmd(offset);
        }
        return FB::buildPLCReadCmd(0);
    }
    }

    return QByteArray();
}

// ========== Extract Frame ==========

QByteArray SiemensProtocol::extractFrame(QByteArray& buffer)
{
    return SiemensFrameBuilder::extractFrame(buffer);
}

// ========== Parse Response ==========

ParsedResponse SiemensProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse result;
    result.cmdCode = 0;
    result.isValid = false;

    if (frame.size() < 4)
        return result;

    // Check TPKT header
    if (static_cast<quint8>(frame[0]) != 0x03)
        return result;

    result.isValid = true;

    // Check if this is a handshake response
    // Handshake 1 response: COTP Connect Confirm (byte[5]==0xD0)
    if (frame.size() >= 6 && static_cast<quint8>(frame[5]) == 0xD0) {
        result.cmdCode = 0xFF;
        result.rawData = frame;
        return result;
    }

    // S7 response: byte[5]==0xF0, byte[7]==0x32
    if (frame.size() >= 8 &&
        static_cast<quint8>(frame[5]) == 0xF0 &&
        static_cast<quint8>(frame[7]) == 0x32) {

        // Handshake 2/3 responses: ROSCTR at byte[8]==0x03 (Ack Data)
        if (frame.size() >= 9 && static_cast<quint8>(frame[8]) == 0x03) {
            // Could be handshake 2 or 3 response
            if (m_handshakeStep == 2) {
                result.cmdCode = 0xFE;
            } else if (m_handshakeStep == 3) {
                result.cmdCode = 0xFD;
            } else {
                result.cmdCode = m_lastRequestCmdCode;
            }
            result.rawData = frame;
            return result;
        }

        // Regular data response - use m_lastRequestCmdCode
        result.cmdCode = m_lastRequestCmdCode;
        result.rawData = frame;
        return result;
    }

    // Fallback
    result.cmdCode = m_lastRequestCmdCode;
    result.rawData = frame;
    return result;
}

// ========== Interpret Data ==========

QString SiemensProtocol::interpretDataRaw(quint8 cmdCode, const QByteArray& data)
{
    using FB = SiemensFrameBuilder;

    auto hexDump = [](const QByteArray& ba) -> QString {
        QStringList hex;
        for (int i = 0; i < ba.size(); ++i)
            hex << QString("%1").arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return hex.join(" ");
    };

    if (data.isEmpty())
        return QStringLiteral("(无数据)");

    // Handshake responses
    if (cmdCode == 0xFF) return QStringLiteral("[握手1/3] COTP连接确认");
    if (cmdCode == 0xFE) return QStringLiteral("[握手2/3] S7通信协商确认");
    if (cmdCode == 0xFD) return QStringLiteral("[握手3/3] S7功能协商完成");

    SiemensCommandDef def = commandDefRaw(cmdCode);

    switch (cmdCode) {
    // String 类型
    case 0x01: case 0x02: case 0x03: case 0x04:
    case 0x11: case 0x12: case 0x23:
        return QStringLiteral("%1: %2").arg(def.name).arg(FB::parseString(data));

    // Mode 类型
    case 0x05:
        return QStringLiteral("操作模式: %1").arg(FB::parseMode(data));

    // Status 类型
    case 0x06:
        return QStringLiteral("运行状态: %1").arg(FB::parseStatus(data));

    // Double 类型
    case 0x07: case 0x08: case 0x09:
    case 0x0A: case 0x0B: case 0x0C:
    case 0x0D: case 0x0E: case 0x0F: case 0x10:
    case 0x16: case 0x17: case 0x18: case 0x19:
    case 0x1A: case 0x1B: case 0x1C:
    case 0x1D: case 0x1E: case 0x1F:
    case 0x20: case 0x21: case 0x22:
    case 0x2F:
        return QStringLiteral("%1: %2").arg(def.name).arg(FB::parseDouble(data), 0, 'f', 3);

    // Int16 类型
    case 0x13: case 0x14: case 0x15:
        return QStringLiteral("%1: %2").arg(def.name).arg(FB::parseInt16(data));

    // Float 类型
    case 0x24: case 0x25: case 0x26:
    case 0x27: case 0x28: case 0x29:
    case 0x2A: case 0x2B: case 0x2C:
    case 0x31:
        return QStringLiteral("%1: %2").arg(def.name).arg(FB::parseFloat(data), 0, 'f', 2);

    // Int16 类型 - 工件坐标系
    case 0x32: case 0x33:
        return QStringLiteral("%1: %2").arg(def.name).arg(FB::parseInt16(data));

    // PLC 告警 - String
    case 0x34:
        return QStringLiteral("PLC告警: %1").arg(FB::parseString(data));

    // Int32/告警 类型
    case 0x2D:
        return QStringLiteral("告警数量: %1").arg(FB::parseInt32(data));
    case 0x2E:
        return QStringLiteral("告警信息: %1").arg(FB::parseInt32(data));

    default:
        return QStringLiteral("HEX: %1").arg(hexDump(data));
    }
}

QString SiemensProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    return interpretDataRaw(cmdCode, data);
}

// ========== Mock Response ==========

// S7 read response layout (what C# code expects):
//   Bytes 0-3:  TPKT header (03 00 LL LL)
//   Byte 4:     0x02 (COTP LI)
//   Byte 5:     0xF0 (COTP Data)
//   Bytes 6-7:  0x80 0x32 (S7 protocol)
//   Byte 8:     0x03 (Response)
//   Bytes 9-10: 0x00 0x00 (Reserved)
//   Bytes 11-12: PDU Reference
//   Bytes 13-14: Parameter length
//   Bytes 15-16: Data length
//   Bytes 17-18: Error class/code
//   Byte 19:    0x04 (Read function)
//   Byte 20:    Item count
//   --- For each data item (starts at byte 21): ---
//   Byte 21+:   0xFF (return code) + transport size + data length + data
//   The first item's data starts at byte 25.
//
// For Mode/Status responses (2 items):
//   Byte 24 = item1 data length low (must be 0x02)
//   Byte 25 = item1 data byte 0 (mode/status value)
//   Byte 26 = item1 data byte 1
//   Byte 27 = item2 return code
//   Byte 28 = item2 transport size
//   Byte 29-30 = item2 data length
//   Byte 31 = item2 data byte 0

QByteArray SiemensProtocol::mockResponseData(quint8 cmdCode, const QByteArray& requestData)
{
    Q_UNUSED(requestData);

    // Handshake responses
    if (cmdCode == 0xFF) {
        return QByteArray::fromHex("0300000b11d00000000100");
    }
    if (cmdCode == 0xFE) {
        return QByteArray::fromHex("0300001902f08032030000010000080000f0000001000103c0");
    }
    if (cmdCode == 0xFD) {
        return QByteArray::fromHex("0300002502f080320300000100000c00000004011108820100140001"
                                   "3b010300000702f000");
    }

    // Helper: build single-item S7 read response
    // Header = 25 bytes, then itemData
    auto buildSingleReadResponse = [&](const QByteArray& itemData) -> QByteArray {
        int paramLen = 1;
        int dataLen = 1 + 1 + 1 + 2 + itemData.size();
        int totalLen = 25 + itemData.size();

        QByteArray frame;
        frame.append('\x03'); frame.append('\x00');
        frame.append(static_cast<char>((totalLen >> 8) & 0xFF));
        frame.append(static_cast<char>(totalLen & 0xFF));
        frame.append('\x02'); frame.append('\xf0');
        frame.append('\x80'); frame.append('\x32');
        frame.append('\x03');
        frame.append('\x00'); frame.append('\x00');
        frame.append('\x00'); frame.append('\x01');
        frame.append(static_cast<char>((paramLen >> 8) & 0xFF));
        frame.append(static_cast<char>(paramLen & 0xFF));
        frame.append(static_cast<char>((dataLen >> 8) & 0xFF));
        frame.append(static_cast<char>(dataLen & 0xFF));
        frame.append('\x00'); frame.append('\x00');
        frame.append('\x04');
        frame.append('\x01');
        frame.append('\xff');
        frame.append(itemData.size() <= 2 ? '\x04' : '\x05');
        frame.append(static_cast<char>((itemData.size() >> 8) & 0xFF));
        frame.append(static_cast<char>(itemData.size() & 0xFF));
        frame.append(itemData);
        return frame;
    };

    // Helper wrappers using capture
    auto buildDoubleResponse = [&](double val) -> QByteArray {
        QByteArray data(8, '\0');
        std::memcpy(data.data(), &val, 8);
        return buildSingleReadResponse(data);
    };

    auto buildFloatResponse = [&](float val) -> QByteArray {
        QByteArray data(4, '\0');
        std::memcpy(data.data(), &val, 4);
        return buildSingleReadResponse(data);
    };

    auto buildInt16Response = [&](qint16 val) -> QByteArray {
        QByteArray data(2, '\0');
        data[0] = static_cast<char>(val & 0xFF);
        data[1] = static_cast<char>((val >> 8) & 0xFF);
        return buildSingleReadResponse(data);
    };

    auto buildInt32Response = [&](qint32 val) -> QByteArray {
        QByteArray data(4, '\0');
        data[0] = static_cast<char>(val & 0xFF);
        data[1] = static_cast<char>((val >> 8) & 0xFF);
        data[2] = static_cast<char>((val >> 16) & 0xFF);
        data[3] = static_cast<char>((val >> 24) & 0xFF);
        return buildSingleReadResponse(data);
    };

    auto buildStringResponse = [&](const QString& str) -> QByteArray {
        return buildSingleReadResponse(str.toUtf8());
    };

    auto buildModeResponse = [&](const QString& mode) -> QByteArray {
        QByteArray frame;
        int totalLen = 32;
        frame.resize(totalLen);
        frame[0] = '\x03'; frame[1] = '\x00';
        frame[2] = static_cast<char>((totalLen >> 8) & 0xFF);
        frame[3] = static_cast<char>(totalLen & 0xFF);
        frame[4] = '\x02'; frame[5] = '\xf0';
        frame[6] = '\x80'; frame[7] = '\x32';
        frame[8] = '\x03';
        frame[9] = '\x00'; frame[10] = '\x00';
        frame[11] = '\x00'; frame[12] = '\x01';
        frame[13] = '\x00'; frame[14] = '\x02';
        int dataLen = totalLen - 4 - 2 - 12 - 2;
        frame[15] = static_cast<char>((dataLen >> 8) & 0xFF);
        frame[16] = static_cast<char>(dataLen & 0xFF);
        frame[17] = '\x00'; frame[18] = '\x00';
        frame[19] = '\x04'; frame[20] = '\x02';
        frame[21] = '\xff'; frame[22] = '\x04';
        frame[23] = '\x00'; frame[24] = '\x02';
        quint8 modeVal = 0x02, refVal = 0x00;
        if (mode == "JOG") { modeVal = 0x00; refVal = 0x00; }
        else if (mode == "MDA") { modeVal = 0x01; refVal = 0x00; }
        else if (mode == "AUTO") { modeVal = 0x02; refVal = 0x00; }
        else if (mode == "REFPOINT") { modeVal = 0x00; refVal = 0x03; }
        frame[25] = static_cast<char>(modeVal);
        frame[26] = '\x00';
        frame[27] = '\xff'; frame[28] = '\x04';
        frame[29] = '\x00'; frame[30] = '\x02';
        frame[31] = static_cast<char>(refVal);
        return frame;
    };

    auto buildStatusResponse = [&](const QString& status) -> QByteArray {
        QByteArray frame;
        int totalLen = 32;
        frame.resize(totalLen);
        frame[0] = '\x03'; frame[1] = '\x00';
        frame[2] = static_cast<char>((totalLen >> 8) & 0xFF);
        frame[3] = static_cast<char>(totalLen & 0xFF);
        frame[4] = '\x02'; frame[5] = '\xf0';
        frame[6] = '\x80'; frame[7] = '\x32';
        frame[8] = '\x03';
        frame[9] = '\x00'; frame[10] = '\x00';
        frame[11] = '\x00'; frame[12] = '\x01';
        frame[13] = '\x00'; frame[14] = '\x02';
        int dataLen = totalLen - 4 - 2 - 12 - 2;
        frame[15] = static_cast<char>((dataLen >> 8) & 0xFF);
        frame[16] = static_cast<char>(dataLen & 0xFF);
        frame[17] = '\x00'; frame[18] = '\x00';
        frame[19] = '\x04'; frame[20] = '\x02';
        frame[21] = '\xff'; frame[22] = '\x04';
        frame[23] = '\x00'; frame[24] = '\x02';
        quint8 b25 = 0x01, b31 = 0x03;
        if (status == "RESET") { b25 = 0x00; b31 = 0x05; }
        else if (status == "STOP") { b25 = 0x02; b31 = 0x02; }
        else if (status == "START") { b25 = 0x01; b31 = 0x03; }
        else if (status == "SPINDLE_CW_CCW") { b25 = 0x01; b31 = 0x05; }
        frame[25] = static_cast<char>(b25);
        frame[26] = '\x00';
        frame[27] = '\xff'; frame[28] = '\x04';
        frame[29] = '\x00'; frame[30] = '\x02';
        frame[31] = static_cast<char>(b31);
        return frame;
    };

    switch (cmdCode) {
    // String 类型
    case 0x01: return buildStringResponse("SIEMENS-828D");
    case 0x02: return buildStringResponse("SINUMERIK 828D");
    case 0x03: return buildStringResponse("V04.06.02");
    case 0x04: return buildStringResponse("2025-01-15");
    // Mode
    case 0x05: return buildModeResponse("AUTO");
    // Status
    case 0x06: return buildStatusResponse("START");
    // Double 类型 - 主轴/进给
    case 0x07: return buildDoubleResponse(2000.0);
    case 0x08: return buildDoubleResponse(1500.0);
    case 0x09: return buildDoubleResponse(100.0);
    case 0x0A: return buildDoubleResponse(800.0);
    case 0x0B: return buildDoubleResponse(500.0);
    case 0x0C: return buildDoubleResponse(120.0);
    // Double 类型 - 计数/时间
    case 0x0D: return buildDoubleResponse(42.0);
    case 0x0E: return buildDoubleResponse(50.0);
    case 0x0F: return buildDoubleResponse(3600.0);
    case 0x10: return buildDoubleResponse(1800.0);
    // String 类型 - 程序
    case 0x11: return buildStringResponse("O1234.MPF");
    case 0x12: return buildStringResponse("N10 G90 G54\nN20 T1 D1\nN30 M03 S1000");
    // Int16 类型 - 刀具
    case 0x13: return buildInt16Response(5);
    case 0x14: return buildInt16Response(1);
    case 0x15: return buildInt16Response(2);
    // Double 类型 - 刀具补偿
    case 0x16: return buildDoubleResponse(0.5);
    case 0x17: return buildDoubleResponse(-1.2);
    case 0x18: return buildDoubleResponse(0.01);
    case 0x19: return buildDoubleResponse(3.0);
    // Double 类型 - 坐标
    case 0x1A: return buildDoubleResponse(100.500);
    case 0x1B: return buildDoubleResponse(200.300);
    case 0x1C: return buildDoubleResponse(-50.000);
    case 0x1D: return buildDoubleResponse(110.500);
    case 0x1E: return buildDoubleResponse(210.300);
    case 0x1F: return buildDoubleResponse(-40.000);
    case 0x20: return buildDoubleResponse(10.000);
    case 0x21: return buildDoubleResponse(9.700);
    case 0x22: return buildDoubleResponse(-10.000);
    case 0x23: return buildStringResponse("X Y Z");
    // Float 类型 - 驱动诊断
    case 0x24: return buildFloatResponse(380.5f);
    case 0x25: return buildFloatResponse(12.8f);
    case 0x26: return buildFloatResponse(3.5f);
    case 0x27: return buildFloatResponse(45.0f);
    case 0x28: return buildFloatResponse(52.3f);
    case 0x29: return buildFloatResponse(38.7f);
    case 0x2A: return buildFloatResponse(65.0f);
    case 0x2B: return buildFloatResponse(58.5f);
    case 0x2C: return buildFloatResponse(42.3f);
    // Int32 类型 - 告警
    case 0x2D: return buildInt32Response(0);
    case 0x2E: return buildInt32Response(0);
    // Double 类型 - 变量
    case 0x2F: return buildDoubleResponse(123.456);
    case 0x30: return buildDoubleResponse(0.0);
    case 0x31: return buildFloatResponse(25.6f);
    // Int16 类型 - 工件坐标系
    case 0x32: return buildInt16Response(0);
    case 0x33: return buildInt16Response(0);
    // PLC 告警 - 返回空字符串表示无告警
    case 0x34: return buildStringResponse("");
    default:
        return QByteArray();
    }
}

QByteArray SiemensProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    return mockResponseData(cmdCode, requestData);
}
