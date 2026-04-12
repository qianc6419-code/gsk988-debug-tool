#include "gsk988protocol.h"
#include "gsk988framebuilder.h"
#include <QDataStream>
#include <QtEndian>
#include <cstring>

// Helper: read little-endian quint16 from byte array at offset
static quint16 readU16(const QByteArray& ba, int offset)
{
    return static_cast<quint8>(ba[offset]) |
           (static_cast<quint8>(ba[offset + 1]) << 8);
}

// Helper: read little-endian quint32 from byte array at offset
static quint32 readU32(const QByteArray& ba, int offset)
{
    return static_cast<quint8>(ba[offset]) |
           (static_cast<quint8>(ba[offset + 1]) << 8) |
           (static_cast<quint8>(ba[offset + 2]) << 16) |
           (static_cast<quint8>(ba[offset + 3]) << 24);
}

// Helper: write little-endian quint16 to byte array
static void appendU16(QByteArray& ba, quint16 v)
{
    ba.append(static_cast<char>(v & 0xFF));
    ba.append(static_cast<char>((v >> 8) & 0xFF));
}

// Helper: write little-endian quint32 to byte array
static void appendU32(QByteArray& ba, quint32 v)
{
    ba.append(static_cast<char>(v & 0xFF));
    ba.append(static_cast<char>((v >> 8) & 0xFF));
    ba.append(static_cast<char>((v >> 16) & 0xFF));
    ba.append(static_cast<char>((v >> 24) & 0xFF));
}

static float readFloat(const QByteArray& ba, int offset)
{
    // Protocol is little-endian, x86 is little-endian — just memcpy
    float f;
    std::memcpy(&f, ba.constData() + offset, sizeof(f));
    return f;
}

static QByteArray floatToBytes(float f)
{
    QByteArray ba(4, '\0');
    std::memcpy(ba.data(), &f, 4);
    return ba;
}

// ========== Command Definitions ==========

static QVector<Gsk988CommandDef> buildCommandTable()
{
    QVector<Gsk988CommandDef> cmds;

    auto add = [&](quint8 code, const QString& name, const QString& cat,
                   const QString& reqP, const QString& respD) {
        cmds.append({code, name, cat, reqP, respD});
    };

    // 通讯管理
    add(0x0A, "获取通讯权限",     "通讯管理", "无",           "错误码+读写权限(1B)");
    add(0x0B, "获取CNC型号",      "通讯管理", "无",           "错误码+型号字符串");

    // 实时加工信息
    add(0x10, "运行模式",         "实时信息", "通道号(1B)",   "错误码+模式(4B)");
    add(0x11, "运行状态",         "实时信息", "通道号(1B)",   "错误码+状态(4B)");
    add(0x12, "运行程序名",       "实时信息", "通道号(1B)",   "错误码+文件名长度(2B)+文件名");
    add(0x13, "有效进给速度",     "实时信息", "通道号(1B)",   "错误码+速度(4B)");
    add(0x14, "有效进给轴名",     "实时信息", "通道号(1B)+轴类型(1B)", "错误码+轴数量(2B)+轴名列表");
    add(0x15, "有效进给轴值(单精度)", "实时信息", "通道号(1B)+轴类型(1B)", "错误码+值个数(2B)+float列表");
    add(0x16, "加工件数",         "实时信息", "通道号(1B)",   "错误码+件数(4B)");
    add(0x17, "当前刀号",         "实时信息", "通道号(1B)",   "错误码+刀号(2B)+刀补号(2B)");
    add(0x18, "运行时间",         "实时信息", "通道号(1B)",   "错误码+时间(4B,秒)");
    add(0x19, "切削时间",         "实时信息", "通道号(1B)",   "错误码+时间(4B,秒)");
    add(0x1A, "速度参数",         "实时信息", "速度类型(1B)+轴号(1B)+通道号(1B)", "错误码+值(4B)");
    add(0x1B, "G模态信息",        "实时信息", "通道号(1B)",   "错误码+G模态数(2B)+模态值列表");
    add(0x1C, "M模态信息",        "实时信息", "通道号(1B)",   "错误码+M模态数(2B)+模态值列表");
    add(0x1D, "当前段号(N)",      "实时信息", "通道号(1B)",   "错误码+段号(4B)");
    add(0x1E, "最小分辨率",       "实时信息", "通道号(1B)",   "错误码+分辨率(4B)");
    add(0x1F, "加工单位",         "实时信息", "通道号(1B)",   "错误码+单位数(2B)+单位列表");
    add(0x20, "运行程序内容",     "实时信息", "通道号(1B)",   "错误码+文件名长度(2B)+内容");
    add(0x21, "当前坐标",         "实时信息", "通道号(1B)",   "错误码+轴数(4B)+坐标列表");
    add(0x22, "暂停状态",         "实时信息", "通道号(1B)",   "错误码+状态(4B)");
    add(0x23, "CNC执行行号",      "实时信息", "通道号(1B)",   "错误码+行号(4B)");
    add(0x24, "有效进给轴值(双精度)", "实时信息", "通道号(1B)+轴类型(1B)", "错误码+值个数(2B)+double列表");
    add(0xAA, "CNC正在执行行号",  "实时信息", "通道号(1B)",   "错误码+行号信息");

    // 参数读取
    add(0x30, "读取参数值",       "参数",     "参数号(2B)+轴/刀补号(1B)", "错误码+参数值(4B)");

    // 刀补读取
    add(0x40, "刀补数量",         "刀补",     "无",           "错误码+数量(4B)");
    add(0x41, "读取刀补值",       "刀补",     "通道号(1B)+刀补号(2B)+组号(1B)+值类型(1B)", "错误码+值(4B)");
    add(0x43, "读取刀补磨损值",   "刀补",     "通道号(1B)+刀补号(2B)+组号(1B)+值类型(1B)", "错误码+值(4B)");

    // 宏变量读取
    add(0x50, "读取宏变量",       "宏变量",   "通道号(1B)+变量号(2B)", "错误码+值(4B)+值类型(1B)");

    // 丝杠螺距补偿
    add(0x60, "螺距补偿数量",     "丝杠补偿", "无",           "错误码+数量(4B)");
    add(0x61, "读取螺距补偿值",   "丝杠补偿", "补偿号",       "错误码+值(4B)");

    // 轴/IO读取
    add(0x70, "当前使用轴名",     "轴IO",     "无",           "错误码+轴信息");
    add(0x71, "读取轴号",         "轴IO",     "通道号(1B)",   "错误码+轴号列表");
    add(0x72, "读取IO状态",       "轴IO",     "无",           "错误码+IO数据");
    add(0x74, "读取轴指定IO点数", "轴IO",     "无",           "错误码+数据");
    add(0x76, "读取轴内部运转状态", "轴IO",   "通道号(1B)",   "错误码+状态");
    add(0x77, "读取轴节点状态",   "轴IO",     "通道号(1B)",   "错误码+状态");
    add(0x79, "读取IO点",         "轴IO",     "IO类型",       "错误码+IO值");
    add(0x7A, "读取轴节点地址",   "轴IO",     "通道号(1B)",   "错误码+地址数据");

    // 报警诊断
    add(0x80, "读取诊断值",       "报警诊断", "诊断号(2B)+轴/刀补号(1B)", "错误码+诊断值");
    add(0x81, "CNC报警数",        "报警诊断", "无",           "错误码+错误数(2B)+警告数(2B)");
    add(0x82, "报警详细信息",     "报警诊断", "索引",         "错误码+报警号+类型+文本");
    add(0x83, "报警历史记录数",   "报警诊断", "无",           "错误码+记录数(4B)");
    add(0x84, "一条报警历史记录", "报警诊断", "索引",         "错误码+记录信息");
    add(0x85, "刀偏修改记录数",   "报警诊断", "无",           "错误码+记录数");
    add(0x86, "一条刀偏修改记录", "报警诊断", "索引",         "错误码+记录信息");

    // 其他读取
    add(0x90, "CNC运行文件个数",  "其他",     "无",           "错误码+文件数");
    add(0xA0, "伺服调试数据",     "其他",     "无",           "错误码+伺服数据");
    add(0xA5, "查询PLC寄存器",    "其他",     "寄存器参数",   "错误码+PLC数据");

    return cmds;
}

// ========== Raw Command Table (internal) ==========

QVector<Gsk988CommandDef> Gsk988Protocol::allCommandsRaw()
{
    static QVector<Gsk988CommandDef> cmds = buildCommandTable();
    return cmds;
}

Gsk988CommandDef Gsk988Protocol::commandDefRaw(quint8 cmdCode)
{
    auto cmds = allCommandsRaw();
    for (const auto& c : cmds) {
        if (c.cmdCode == cmdCode)
            return c;
    }
    return {cmdCode, "未知命令", "其他", "", ""};
}

// ========== IProtocol: allCommands / commandDef ==========

QVector<ProtocolCommand> Gsk988Protocol::allCommands() const
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

ProtocolCommand Gsk988Protocol::commandDef(quint8 cmdCode) const
{
    Gsk988CommandDef raw = commandDefRaw(cmdCode);
    ProtocolCommand pc;
    pc.cmdCode = raw.cmdCode;
    pc.name = raw.name;
    pc.category = raw.category;
    pc.paramDesc = raw.requestParams;
    pc.respDesc = raw.responseDesc;
    return pc;
}

// ========== Constructor ==========

Gsk988Protocol::Gsk988Protocol(QObject* parent)
    : IProtocol(parent)
{
}

// ========== Build Request ==========

QByteArray Gsk988Protocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    QByteArray dataField;
    dataField.append(static_cast<char>(cmdCode));
    dataField.append(params);
    return Gsk988FrameBuilder::buildRequestFrame(dataField);
}

// ========== Parse Response ==========

ParsedResponse Gsk988Protocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse result;

    if (!Gsk988FrameBuilder::validateFrame(frame)) {
        result.isValid = false;
        return result;
    }

    // Skip: head(2) + length(2) + response_id(8) = 12
    const int dataOffset = 12;

    if (frame.size() < dataOffset + 3) {
        result.isValid = false;
        return result;
    }

    result.cmdCode = static_cast<quint8>(frame[dataOffset]);

    quint16 errorCode = readU16(frame, dataOffset + 1);
    result.isValid = (errorCode == 0);

    // rawData = payload after cmdCode + errorCode
    result.rawData = frame.mid(dataOffset + 3);

    return result;
}

// ========== Extract Frame ==========

QByteArray Gsk988Protocol::extractFrame(QByteArray& buffer)
{
    return Gsk988FrameBuilder::extractFrame(buffer);
}

// ========== Interpret Data ==========

QString Gsk988Protocol::interpretDataRaw(quint8 cmdCode, const QByteArray& data)
{
    if (data.isEmpty())
        return "(无附加数据)";

    switch (cmdCode) {
    case 0x0A: {
        if (data.size() >= 1) {
            quint8 perm = static_cast<quint8>(data[0]);
            QString desc;
            if (perm == 0) desc = "无权限";
            else if (perm == 1) desc = "只读";
            else if (perm == 2) desc = "可读写";
            else desc = QString("未知(%1)").arg(perm);
            return QString("权限值: %1 (%2)").arg(perm).arg(desc);
        }
        break;
    }
    case 0x0B: {
        return QString("型号: %1").arg(QString::fromUtf8(data));
    }
    case 0x10: {
        if (data.size() >= 4) {
            quint32 mode = readU32(data, 0);
            QString modeStr;
            switch (mode) {
            case 0: modeStr = "手动"; break;
            case 1: modeStr = "自动"; break;
            case 2: modeStr = "MDI"; break;
            case 3: modeStr = "自动(3)"; break;
            default: modeStr = QString("未知(%1)").arg(mode); break;
            }
            return QString("运行模式: %1 (%2)").arg(mode).arg(modeStr);
        }
        break;
    }
    case 0x11: {
        if (data.size() >= 4) {
            quint32 status = readU32(data, 0);
            QString statusStr;
            switch (status) {
            case 0: statusStr = "停止"; break;
            case 1: statusStr = "运行中"; break;
            case 2: statusStr = "暂停"; break;
            default: statusStr = QString("未知(%1)").arg(status); break;
            }
            return QString("运行状态: %1 (%2)").arg(status).arg(statusStr);
        }
        break;
    }
    case 0x16: {
        if (data.size() >= 4) {
            quint32 count = readU32(data, 0);
            return QString("加工件数: %1").arg(count);
        }
        break;
    }
    case 0x17: {
        if (data.size() >= 4) {
            quint16 toolNo = readU16(data, 0);
            quint16 compNo = readU16(data, 2);
            return QString("刀号: %1, 刀补号: %2").arg(toolNo).arg(compNo);
        }
        break;
    }
    case 0x18: {
        if (data.size() >= 4) {
            quint32 secs = readU32(data, 0);
            int h = secs / 3600;
            int m = (secs % 3600) / 60;
            int s = secs % 60;
            return QString("运行时间: %1h %2m %3s (%4秒)").arg(h).arg(m).arg(s).arg(secs);
        }
        break;
    }
    case 0x19: {
        if (data.size() >= 4) {
            quint32 secs = readU32(data, 0);
            int h = secs / 3600;
            int m = (secs % 3600) / 60;
            int s = secs % 60;
            return QString("切削时间: %1h %2m %3s (%4秒)").arg(h).arg(m).arg(s).arg(secs);
        }
        break;
    }
    case 0x21: {
        if (data.size() >= 4) {
            quint32 axisCount = readU32(data, 0);
            int offset = 4;
            QStringList coords;
            for (quint32 i = 0; i < axisCount && offset + 4 <= data.size(); ++i) {
                float val = readFloat(data, offset);
                coords << QString("%1").arg(val, 0, 'f', 3);
                offset += 4;
            }
            return QString("坐标 (轴数=%1): %2").arg(axisCount).arg(coords.join(", "));
        }
        break;
    }
    case 0x81: {
        if (data.size() >= 4) {
            quint16 errCount = readU16(data, 0);
            quint16 warnCount = readU16(data, 2);
            return QString("错误数: %1, 警告数: %2").arg(errCount).arg(warnCount);
        }
        break;
    }
    default: {
        // Return hex dump for unhandled commands
        QStringList hex;
        for (int i = 0; i < data.size(); ++i) {
            hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        }
        return QString("HEX: %1").arg(hex.join(" "));
    }
    }

    return QString("(数据长度: %1字节)").arg(data.size());
}

QString Gsk988Protocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    return interpretDataRaw(cmdCode, data);
}

// ========== Mock Response Data (internal) ==========

QByteArray Gsk988Protocol::mockResponseData(quint8 cmdCode, const QByteArray& requestData)
{
    QByteArray data;

    // All responses: cmdCode(1B) + errorCode(2B, 0x0000) + additional data
    data.append(static_cast<char>(cmdCode));
    data.append('\x00'); // error code high
    data.append('\x00'); // error code low

    switch (cmdCode) {
    case 0x0A:
        data.append('\x02'); // permission = read/write
        break;
    case 0x0B:
        data.append("GSK988TD");
        break;
    case 0x10:
        appendU32(data, 3); // mode = auto
        break;
    case 0x11:
        appendU32(data, 1); // status = running
        break;
    case 0x12: {
        QByteArray name = "O0001.NC";
        appendU16(data, static_cast<quint16>(name.size()));
        data.append(name);
        break;
    }
    case 0x13:
        appendU32(data, 1500); // feed speed
        break;
    case 0x14: {
        appendU16(data, 3); // 3 axes
        data.append("X");
        data.append('\0');
        data.append("Y");
        data.append('\0');
        data.append("Z");
        data.append('\0');
        break;
    }
    case 0x15: {
        // Axis type from request: 0=机械坐标, 1=绝对坐标, 2=相对坐标, 3=余移动量
        quint8 axisType = (requestData.size() >= 2) ? static_cast<quint8>(requestData[1]) : 0;
        float x = 0, y = 0, z = 0;
        switch (axisType) {
        case 0: x = 123.456f; y = -45.678f; z = 50.0f; break;   // 机械坐标
        case 1: x = 100.0f;   y = -50.0f;   z = 25.0f; break;   // 绝对坐标
        case 2: x = 10.5f;    y = -5.5f;    z = 0.0f;  break;   // 相对坐标
        case 3: x = 5.25f;    y = -2.75f;   z = 0.0f;  break;   // 余移动量
        default: break;
        }
        appendU16(data, 3);
        data.append(floatToBytes(x));
        data.append(floatToBytes(y));
        data.append(floatToBytes(z));
        break;
    }
    case 0x16:
        appendU32(data, 42); // piece count
        break;
    case 0x17:
        appendU16(data, 1); // tool no
        appendU16(data, 1); // tool comp no
        break;
    case 0x18:
        appendU32(data, 3661); // runtime seconds (1h 1m 1s)
        break;
    case 0x19:
        appendU32(data, 1830); // cutting time
        break;
    case 0x1A: {
        // Speed values are float, not uint32
        float val = 0;
        if (requestData.size() >= 1) {
            quint8 speedType = static_cast<quint8>(requestData[0]);
            switch (speedType) {
            case 0: val = 1000.0f; break;  // 进给编程速度
            case 1: val = 800.0f;  break;  // 进给实际速度
            case 2: val = 100.0f;  break;  // 进给倍率
            case 3: val = 1500.0f; break;  // 主轴编程速度
            case 4: val = 1200.0f; break;  // 主轴实际速度
            case 5: val = 80.0f;   break;  // 主轴倍率
            case 6: val = 100.0f;  break;  // 快速倍率
            case 7: val = 50.0f;   break;  // 手动倍率
            case 8: val = 10.0f;   break;  // 手轮倍率
            default: val = 0.0f;   break;
            }
        }
        data.append(floatToBytes(val));
        break;
    }
    case 0x1B: {
        appendU16(data, 3);
        appendU16(data, 0); appendU16(data, 1); appendU16(data, 90);
        break;
    }
    case 0x1C: {
        appendU16(data, 2);
        appendU16(data, 3); appendU16(data, 8);
        break;
    }
    case 0x1D:
        appendU32(data, 50);
        break;
    case 0x1E:
        appendU32(data, 1000); // resolution
        break;
    case 0x1F: {
        appendU16(data, 3);
        data.append("mm");
        data.append('\0');
        data.append("mm");
        data.append('\0');
        data.append("mm");
        data.append('\0');
        break;
    }
    case 0x20: {
        QByteArray content = "G90 G54 G00 X0 Y0 Z50";
        appendU16(data, static_cast<quint16>(content.size()));
        data.append(content);
        break;
    }
    case 0x21: {
        appendU32(data, 3); // 3 axes
        data.append(floatToBytes(123.456f));
        data.append(floatToBytes(-45.678f));
        data.append(floatToBytes(0.0f));
        break;
    }
    case 0x22:
        appendU32(data, 0); // not paused
        break;
    case 0x23:
        appendU32(data, 25); // line number
        break;
    case 0x24: {
        appendU16(data, 3);
        // double is 8 bytes, LE on protocol
        double vals[] = {123.456, -45.678, 0.0};
        for (double v : vals) {
            data.append(reinterpret_cast<const char*>(&v), 8);
        }
        break;
    }
    case 0xAA:
        appendU32(data, 25);
        break;
    case 0x30:
        appendU32(data, 100); // parameter value
        break;
    case 0x40:
        appendU32(data, 16); // tool comp count
        break;
    case 0x41:
    case 0x43:
        appendU32(data, 0); // tool comp value
        break;
    case 0x50:
        appendU32(data, 500); // macro var value
        data.append('\x00'); // value type
        break;
    case 0x60:
        appendU32(data, 128); // pitch comp count
        break;
    case 0x61:
        appendU32(data, 0); // pitch comp value
        break;
    case 0x70: {
        QByteArray axisInfo = "X\0Y\0Z";
        data.append(axisInfo);
        break;
    }
    case 0x71: {
        data.append(static_cast<char>(3));
        data.append('\x00'); data.append('\x01'); data.append('\x02');
        break;
    }
    case 0x72:
    case 0x74:
        appendU32(data, 0);
        break;
    case 0x76:
    case 0x77:
        appendU32(data, 0);
        break;
    case 0x79:
        data.append('\x00'); // IO value
        break;
    case 0x7A:
        appendU32(data, 0);
        break;
    case 0x80:
        appendU32(data, 0); // diagnosis value
        break;
    case 0x81:
        appendU16(data, 0); // error count
        appendU16(data, 0); // warning count
        break;
    case 0x82:
        data.append('\x00'); // alarm code
        data.append('\x00'); // type
        data.append("无报警");
        break;
    case 0x83:
        appendU32(data, 0); // alarm history count
        break;
    case 0x84:
    case 0x86:
        data.append("无记录");
        break;
    case 0x85:
        appendU32(data, 0);
        break;
    case 0x90:
        appendU32(data, 5); // file count
        break;
    case 0xA0:
        appendU32(data, 0);
        break;
    case 0xA5:
        data.append('\x00');
        break;
    default:
        // Unknown command — return just cmdCode + errorCode
        break;
    }

    return data;
}

// ========== Mock Response (IProtocol — returns complete frame) ==========

QByteArray Gsk988Protocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    QByteArray responseData = mockResponseData(cmdCode, requestData);
    return Gsk988FrameBuilder::buildResponseFrame(responseData);
}
