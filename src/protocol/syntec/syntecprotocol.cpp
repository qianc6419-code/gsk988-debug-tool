#include "syntecprotocol.h"
#include "syntecframebuilder.h"
#include <QByteArray>
#include <cstring>

static const QVector<ProtocolCommand> s_commands = {
    {0x00, "握手",         "连接", "无", "握手"},
    {0x01, "程序名",       "状态", "无", "当前程序名"},
    {0x02, "操作模式",     "状态", "无", "0=NULL..7=回零"},
    {0x03, "运行状态",     "状态", "无", "0=未就绪..4=停止"},
    {0x04, "报警数量",     "状态", "无", "报警个数"},
    {0x05, "急停状态",     "状态", "无", "0xFF=急停"},
    {0x06, "开机时间",     "时间", "无", "秒"},
    {0x07, "切削时间",     "时间", "无", "秒"},
    {0x08, "循环时间",     "时间", "无", "秒"},
    {0x09, "工件数",       "计数", "无", "当前工件数"},
    {0x0A, "需求工件数",   "计数", "无", "目标工件数"},
    {0x0B, "总工件数",     "计数", "无", "累计总工件数"},
    {0x0C, "进给倍率",     "倍率", "无", "百分比"},
    {0x0D, "主轴倍率",     "倍率", "无", "百分比"},
    {0x0E, "主轴速度",     "速度", "无", "RPM"},
    {0x0F, "进给速度(原始)", "速度", "无", "需除以10^DecPoint"},
    {0x10, "小数位数",     "速度", "无", "DecPoint"},
    {0x20, "相对坐标",     "坐标", "无", "3轴int64/10^DecPoint"},
    {0x21, "机床坐标",     "坐标", "无", "3轴int64/10^DecPoint"},
};

SyntecProtocol::SyntecProtocol(QObject* parent)
    : IProtocol(parent)
{
}

QByteArray SyntecProtocol::buildHandshake()
{
    return buildRequest(0x00);
}

QByteArray SyntecProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    Q_UNUSED(params);
    quint8 tid = nextTid();
    m_pendingTid = tid;
    m_pendingCmdCode = cmdCode;
    return SyntecFrameBuilder::buildPacket(cmdCode, tid);
}

ParsedResponse SyntecProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse resp;
    if (frame.size() < 20) {
        resp.isValid = false;
        return resp;
    }

    quint8 respTid = SyntecFrameBuilder::extractTid(frame);
    if (respTid != m_pendingTid) {
        resp.isValid = false;
        return resp;
    }

    resp.cmdCode = m_pendingCmdCode;
    resp.isValid = SyntecFrameBuilder::checkFrame(frame, m_pendingTid);
    resp.rawData = frame.mid(20);
    return resp;
}

QByteArray SyntecProtocol::extractFrame(QByteArray& buffer)
{
    // Skip empty/keepalive responses
    while (buffer.size() >= 12 && SyntecFrameBuilder::isEmptyResponse(buffer)) {
        buffer.remove(0, 12);
    }

    if (buffer.size() < 4) return QByteArray();

    qint32 totalLen = 0;
    memcpy(&totalLen, buffer.constData(), 4);
    quint32 frameSize = static_cast<quint32>(totalLen) + 4;

    if (static_cast<quint32>(buffer.size()) < frameSize)
        return QByteArray();

    QByteArray frame = buffer.left(frameSize);
    buffer.remove(0, frameSize);
    return frame;
}

QVector<ProtocolCommand> SyntecProtocol::allCommands() const
{
    return s_commands;
}

ProtocolCommand SyntecProtocol::commandDef(quint8 cmdCode) const
{
    for (const auto& cmd : s_commands) {
        if (cmd.cmdCode == cmdCode) return cmd;
    }
    return ProtocolCommand{cmdCode, QStringLiteral("未知命令"), "", "", ""};
}

QString SyntecProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    using FB = SyntecFrameBuilder;
    switch (cmdCode) {
    case 0x01: return QStringLiteral("程序名: %1").arg(FB::extractString(data, 0));
    case 0x02: return QStringLiteral("操作模式: %1 (%2)").arg(FB::modeToString(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x03: return QStringLiteral("运行状态: %1 (%2)").arg(FB::statusToString(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x04: return QStringLiteral("报警数量: %1").arg(FB::extractInt32(data, 0));
    case 0x05: {
        if (data.size() >= 8) {
            int emg = static_cast<quint8>(data[4]);
            return QStringLiteral("急停: %1 (0x%2)").arg(FB::emgToString(emg)).arg(emg, 2, 16, QChar('0'));
        }
        break;
    }
    case 0x06: return QStringLiteral("开机时间: %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x07: return QStringLiteral("切削时间: %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x08: return QStringLiteral("循环时间: %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x09: return QStringLiteral("工件数: %1").arg(FB::extractInt32(data, 0));
    case 0x0A: return QStringLiteral("需求工件数: %1").arg(FB::extractInt32(data, 0));
    case 0x0B: return QStringLiteral("总工件数: %1").arg(FB::extractInt32(data, 0));
    case 0x0C: return QStringLiteral("进给倍率: %1%").arg(FB::extractInt32(data, 0));
    case 0x0D: return QStringLiteral("主轴倍率: %1%").arg(FB::extractInt32(data, 0));
    case 0x0E: return QStringLiteral("主轴速度: %1 RPM").arg(FB::extractInt32(data, 0));
    case 0x0F: return QStringLiteral("进给速度(原始): %1").arg(FB::extractInt32(data, 0));
    case 0x10: return QStringLiteral("小数位数: %1").arg(FB::extractInt32(data, 0));
    case 0x20:
    case 0x21: {
        QString label = (cmdCode == 0x20) ? QStringLiteral("相对") : QStringLiteral("机床");
        QString result = label + QStringLiteral("坐标: ");
        for (int i = 0; i < 3; ++i) {
            qint64 raw = FB::extractInt64(data, i * 8);
            if (i > 0) result += ", ";
            result += QString::number(raw);
        }
        return result;
    }
    default:
        break;
    }

    // Fallback: hex dump
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
    return QStringLiteral("HEX: %1").arg(hex.join(" "));
}

QByteArray SyntecProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    // Use m_pendingTid which was set by buildRequest before this call
    Q_UNUSED(requestData);
    quint8 tid = m_pendingTid;

    // Build response: [4B totalLen][4B vendor=0xC8][2B vendor2=0xC8][1B tid][1B 0x00][4B err1=0][4B err2=0][data...]
    auto buildResp = [](quint8 t, const QByteArray& data) -> QByteArray {
        QByteArray resp;
        qint32 totalLen = 16 + data.size();
        resp.resize(20 + data.size());
        memset(resp.data(), 0, resp.size());
        memcpy(resp.data(), &totalLen, 4);
        qint32 vendor = 0xC8;
        memcpy(resp.data() + 4, &vendor, 4);
        resp[8] = static_cast<char>(0xC8);
        resp[9] = 0;
        resp[10] = static_cast<char>(t);
        resp[11] = 0;
        // err1 and err2 already 0
        memcpy(resp.data() + 20, data.constData(), data.size());
        return resp;
    };

    QByteArray mockData;
    switch (cmdCode) {
    case 0x00: // Handshake - empty data
        break;
    case 0x01: { // ProgramName - encoded string "O1234"
        const char* str = "O1234";
        mockData.resize(20);
        for (int i = 0; str[i]; ++i) {
            mockData[i * 2] = str[i];
        }
        break;
    }
    case 0x02: { // Mode = AUTO(2)
        qint32 v = 2;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x03: { // Status = START(2)
        qint32 v = 2;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x04: { // Alarm = 0
        qint32 v = 0;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x05: { // EMG = normal
        mockData.append(8, '\0');
        break;
    }
    case 0x06: // RunTime = 3600s
    case 0x07: // CutTime = 1800s
    case 0x08: { // CycleTime = 120s
        qint32 v = (cmdCode == 0x06) ? 3600 : (cmdCode == 0x07) ? 1800 : 120;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x09: // Products = 5
    case 0x0A: // RequiredProducts = 100
    case 0x0B: { // TotalProducts = 50
        qint32 v = (cmdCode == 0x09) ? 5 : (cmdCode == 0x0A) ? 100 : 50;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x0C: // FeedOverride = 100%
    case 0x0D: { // SpindleOverride = 80%
        qint32 v = (cmdCode == 0x0C) ? 100 : 80;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x0E: { // SpindleSpeed = 3000 RPM
        qint32 v = 3000;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x0F: { // FeedRateOri = 12000
        qint32 v = 12000;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x10: { // DecPoint = 3
        qint32 v = 3;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x20: // RelativeAxes
    case 0x21: { // MachineAxes - 3 axes, each 8 bytes int64
        for (int i = 0; i < 3; ++i) {
            qint64 val = (i == 0) ? 100000 : (i == 1) ? 200000 : -50000;
            mockData.append(reinterpret_cast<const char*>(&val), 8);
        }
        break;
    }
    default:
        mockData.append(8, '\0');
        break;
    }

    return buildResp(tid, mockData);
}
