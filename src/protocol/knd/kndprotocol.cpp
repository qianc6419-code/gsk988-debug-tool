#include "protocol/knd/kndprotocol.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static const QVector<ProtocolCommand> s_commands = {
    {0x01, "系统信息",       "系统", "无", "型号、版本、轴列表"},
    {0x02, "系统状态",       "系统", "无", "运行状态、模式、报警"},
    {0x03, "报警信息",       "系统", "无", "报警描述"},
    {0x10, "坐标",           "坐标", "无", "绝对/机床/相对/余移动量"},
    {0x11, "驱动负载",       "坐标", "无", "驱动负载"},
    {0x20, "进给倍率",       "倍率", "无", "进给倍率"},
    {0x21, "快速倍率",       "倍率", "无", "快速倍率"},
    {0x22, "主轴倍率",       "倍率", "spNo", "主轴倍率"},
    {0x23, "主轴速率",       "倍率", "无", "主轴速率"},
    {0x24, "实际进给",       "倍率", "无", "实际进给速率"},
    {0x30, "加工时间",       "时间", "无", "循环/加工时间"},
    {0x31, "加工计数",       "时间", "无", "加工计数"},
    {0x32, "目标件数",       "时间", "无", "目标件数"},
    {0x40, "工件坐标系",     "参数", "name", "工件坐标系"},
    {0x50, "宏变量",         "参数", "names", "宏变量"},
    {0x60, "刀补形状",       "刀补", "start,count", "刀补形状"},
    {0x61, "刀补磨损",       "刀补", "start,count", "刀补磨损"},
    {0x70, "G代码模态",      "程序", "无", "G代码模态"},
    {0x80, "程序列表",       "程序", "无", "程序列表"},
    {0x81, "程序内容",       "程序", "no", "程序内容"},
    {0x82, "MDI",            "程序", "content", "MDI 临时程序"},
    {0x90, "PLC数据",        "PLC",  "region,offset,type,count", "PLC数据"},
    {0xA0, "清零相对坐标",   "操作", "无", "清零相对坐标"},
    {0xA1, "清零加工时间",   "操作", "无", "清零加工时间"},
};

KndProtocol::KndProtocol(QObject* parent)
    : IProtocol(parent)
{
}

QByteArray KndProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    Q_UNUSED(cmdCode);
    Q_UNUSED(params);
    return QByteArray(1, cmdCode);
}

ParsedResponse KndProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse resp;
    if (frame.isEmpty()) return resp;
    resp.cmdCode = static_cast<quint8>(frame[0]);
    resp.rawData = frame.mid(1);
    resp.isValid = true;
    return resp;
}

QByteArray KndProtocol::extractFrame(QByteArray& buffer)
{
    QByteArray frame = buffer;
    buffer.clear();
    return frame;
}

QVector<ProtocolCommand> KndProtocol::allCommands() const
{
    return s_commands;
}

ProtocolCommand KndProtocol::commandDef(quint8 cmdCode) const
{
    for (const auto& cmd : s_commands) {
        if (cmd.cmdCode == cmdCode) return cmd;
    }
    return ProtocolCommand{cmdCode, "未知命令", "", "", ""};
}

const QVector<ProtocolCommand>& KndProtocol::staticCommands()
{
    return s_commands;
}

QString KndProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    Q_UNUSED(cmdCode);
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        return QString::fromUtf8(QJsonDocument(doc.object()).toJson(QJsonDocument::Indented));
    } else if (doc.isArray()) {
        return QString::fromUtf8(QJsonDocument(doc.array()).toJson(QJsonDocument::Indented));
    }
    return QString::fromUtf8(data);
}

QByteArray KndProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    Q_UNUSED(requestData);
    QJsonObject mock;

    switch (cmdCode) {
    case 0x02:
        mock["run-status"] = 2;
        mock["opr-mode"] = 1;
        mock["alarm"] = 0;
        mock["exec-no"] = 1234;
        break;
    case 0x10: {
        QJsonArray axes;
        for (const char* name : {"X", "Y", "Z"}) {
            QJsonObject ax;
            ax["name"] = name;
            ax["machine"] = 100.0;
            ax["absolute"] = 50.0;
            ax["relative"] = 20.0;
            ax["distance-to-go"] = 5.0;
            axes.append(ax);
        }
        mock["axis"] = axes;
        break;
    }
    case 0x20: mock["override"] = 100; break;
    case 0x21: mock["override"] = 100; break;
    case 0x22: mock["override"] = 100; break;
    case 0x23: {
        QJsonArray sps;
        QJsonObject sp;
        sp["sp-no"] = 1;
        sp["set-speed"] = 3000;
        sp["act-speed"] = 2980;
        sps.append(sp);
        mock["spindle"] = sps;
        break;
    }
    case 0x24: mock["feed-rate"] = 120.0; break;
    case 0x30:
        mock["cycle-h"] = 0; mock["cycle-m"] = 5; mock["cycle-s"] = 30;
        mock["total-h"] = 12; mock["total-m"] = 20; mock["total-s"] = 0;
        break;
    case 0x31: mock["total"] = 5; break;
    case 0x32: mock["total"] = 100; break;
    default:
        mock["status"] = "ok";
        break;
    }

    return QByteArray(1, cmdCode) + QJsonDocument(mock).toJson(QJsonDocument::Compact);
}