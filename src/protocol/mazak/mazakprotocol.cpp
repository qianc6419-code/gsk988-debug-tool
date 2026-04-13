#include "protocol/mazak/mazakprotocol.h"

static const QVector<MazakCommandDef> s_commands = {
    {0x01, "运行信息",     "运行状态", "获取运行信息(MTC_RUNNING_INFO)"},
    {0x02, "运行状态",     "运行状态", "获取运行状态"},
    {0x03, "运行模式",     "运行状态", "获取运行模式"},
    {0x10, "轴信息",       "坐标",     "获取轴信息(MTC_AXIS_INFO)"},
    {0x11, "当前坐标",     "坐标",     "获取当前坐标(MAZ_NCPOS)"},
    {0x12, "剩余距离",     "坐标",     "获取剩余距离"},
    {0x13, "轴名",         "坐标",     "获取轴名称"},
    {0x20, "主轴信息",     "主轴",     "获取主轴信息(MTC_SPINDLE_INFO)"},
    {0x21, "主轴负载",     "主轴",     "获取主轴负载"},
    {0x22, "实际转速",     "主轴",     "获取实际转速"},
    {0x23, "设定转速",     "主轴",     "获取设定转速"},
    {0x30, "进给速度",     "进给",     "获取进给速度(MAZ_FEED)"},
    {0x40, "当前刀具",     "刀具",     "获取当前刀具信息"},
    {0x41, "刀补数据",     "刀具",     "获取刀补数据"},
    {0x50, "报警",         "报警",     "获取报警信息"},
    {0x60, "主程序",       "程序",     "获取主程序名"},
    {0x61, "运行程序",     "程序",     "获取运行程序名"},
    {0x70, "NC参数读写",   "参数",     "读写NC参数(getRangedParam/setRangedParam)"},
    {0x71, "宏变量读写",   "参数",     "读写宏变量(getRangedCmnVar/setRangedCmnVar)"},
    {0x72, "R寄存器读写",  "参数",     "读写R寄存器(getRangedRReg/setRangedRReg)"},
    {0x73, "工件坐标系",   "参数",     "读写工件坐标系"},
    {0x74, "刀补读写",     "参数",     "读写刀补数据"},
    {0xA0, "累计时间",     "时间",     "获取累计时间"},
    {0xA1, "开机时间",     "时间",     "获取开机时间"},
    {0xA2, "运行时间",     "时间",     "获取运行时间"},
    {0xF0, "NC复位",       "操作",     "执行NC复位"},
};

MazakProtocol::MazakProtocol(QObject* parent)
    : IProtocol(parent)
{
}

const QVector<MazakCommandDef>& MazakProtocol::commandDefs()
{
    return s_commands;
}

QByteArray MazakProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    // Mazak 不走帧发送，返回命令标记
    QByteArray data;
    data.append(static_cast<char>(cmdCode));
    data.append(params);
    return data;
}

ParsedResponse MazakProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse resp;
    if (frame.isEmpty()) return resp;
    resp.cmdCode = static_cast<quint8>(frame[0]);
    resp.rawData = frame.mid(1);
    resp.isValid = true;
    return resp;
}

QByteArray MazakProtocol::extractFrame(QByteArray& buffer)
{
    // Mazak 不做帧提取，DLL 处理
    QByteArray data = buffer;
    buffer.clear();
    return data;
}

QVector<ProtocolCommand> MazakProtocol::allCommands() const
{
    QVector<ProtocolCommand> cmds;
    for (const auto& def : s_commands) {
        ProtocolCommand cmd;
        cmd.cmdCode = def.cmdCode;
        cmd.name = def.name;
        cmd.category = def.category;
        cmd.paramDesc = def.desc;
        cmd.respDesc = def.desc;
        cmds.append(cmd);
    }
    return cmds;
}

ProtocolCommand MazakProtocol::commandDef(quint8 cmdCode) const
{
    for (const auto& def : s_commands) {
        if (def.cmdCode == cmdCode) {
            ProtocolCommand cmd;
            cmd.cmdCode = def.cmdCode;
            cmd.name = def.name;
            cmd.category = def.category;
            cmd.paramDesc = def.desc;
            cmd.respDesc = def.desc;
            return cmd;
        }
    }
    return ProtocolCommand{cmdCode, "未知", "未知", "", ""};
}

QString MazakProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    Q_UNUSED(cmdCode);
    Q_UNUSED(data);
    return "Mazak DLL 数据 — 通过 Widget 直接显示";
}

QByteArray MazakProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    Q_UNUSED(cmdCode);
    Q_UNUSED(requestData);
    return QByteArray();
}
