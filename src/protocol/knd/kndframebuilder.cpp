#include "protocol/knd/kndframebuilder.h"

QString KndFrameBuilder::runStatusToString(int status)
{
    switch (status) {
    case 0:  return "停止";
    case 1:  return "暂停";
    case 2:  return "运行";
    default: return QString("未知(%1)").arg(status);
    }
}

QString KndFrameBuilder::runStatusColor(int status)
{
    switch (status) {
    case 0:  return "color: #2196F3; font-weight: bold;";
    case 1:  return "color: orange; font-weight: bold;";
    case 2:  return "color: green; font-weight: bold;";
    default: return "";
    }
}

QString KndFrameBuilder::oprModeToString(int mode)
{
    switch (mode) {
    case 0:  return "录入";
    case 1:  return "自动";
    case 3:  return "编辑";
    case 4:  return "单步";
    case 5:  return "手动";
    case 6:  return "手动编辑";
    case 7:  return "手轮编辑";
    case 8:  return "手轮";
    case 9:  return "回零";
    case 10: return "程序回零";
    default: return QString("未知(%1)").arg(mode);
    }
}

QVector<KndFrameBuilder::AxisCoord> KndFrameBuilder::parseCoordinates(const QJsonObject& obj)
{
    QVector<AxisCoord> result;
    QJsonArray axes = obj["axis"].toArray();
    for (const auto& a : axes) {
        QJsonObject o = a.toObject();
        AxisCoord c;
        c.name = o["name"].toString();
        c.machine = o["machine"].toDouble();
        c.absolute = o["absolute"].toDouble();
        c.relative = o["relative"].toDouble();
        c.distanceToGo = o["distance-to-go"].toDouble();
        result.append(c);
    }
    return result;
}

QVector<KndFrameBuilder::AxisLoad> KndFrameBuilder::parseAxisLoad(const QJsonObject& obj)
{
    QVector<AxisLoad> result;
    QJsonArray axes = obj["axis"].toArray();
    for (const auto& a : axes) {
        QJsonObject o = a.toObject();
        AxisLoad l;
        l.name = o["name"].toString();
        l.load = o["load"].toDouble();
        result.append(l);
    }
    return result;
}

QVector<KndFrameBuilder::SpindleSpeed> KndFrameBuilder::parseSpindleSpeeds(const QJsonObject& obj)
{
    QVector<SpindleSpeed> result;
    QJsonArray sps = obj["spindle"].toArray();
    for (const auto& s : sps) {
        QJsonObject o = s.toObject();
        SpindleSpeed sp;
        sp.spNo = o["sp-no"].toInt();
        sp.setSpeed = o["set-speed"].toDouble();
        sp.actSpeed = o["act-speed"].toDouble();
        result.append(sp);
    }
    return result;
}

int KndFrameBuilder::parseOverride(const QJsonObject& obj)
{
    return obj["override"].toInt();
}

QVector<KndFrameBuilder::AlarmInfo> KndFrameBuilder::parseAlarms(const QJsonObject& obj)
{
    QVector<AlarmInfo> result;
    QJsonArray alarms = obj["alarms"].toArray();
    for (const auto& a : alarms) {
        QJsonObject o = a.toObject();
        AlarmInfo ai;
        ai.no = o["no"].toInt();
        ai.message = o["message"].toString();
        result.append(ai);
    }
    return result;
}

KndFrameBuilder::CycleTime KndFrameBuilder::parseCycleTime(const QJsonObject& obj)
{
    CycleTime ct;
    ct.cycleHour = obj["cycle-h"].toInt();
    ct.cycleMin  = obj["cycle-m"].toInt();
    ct.cycleSec  = obj["cycle-s"].toInt();
    ct.totalHour = obj["total-h"].toInt();
    ct.totalMin  = obj["total-m"].toInt();
    ct.totalSec  = obj["total-s"].toInt();
    return ct;
}

QVector<KndFrameBuilder::ToolOffset> KndFrameBuilder::parseToolOffsets(const QJsonArray& arr)
{
    QVector<ToolOffset> result;
    for (const auto& item : arr) {
        QJsonObject o = item.toObject();
        ToolOffset to;
        to.ofsNo = o["ofs-no"].toInt();
        QJsonObject vals = o["values"].toObject();
        for (auto it = vals.begin(); it != vals.end(); ++it) {
            to.values[it.key()] = it.value().toDouble();
        }
        result.append(to);
    }
    return result;
}

QVector<KndFrameBuilder::WorkCoord> KndFrameBuilder::parseWorkCoords(const QJsonObject& obj)
{
    QVector<WorkCoord> result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        WorkCoord wc;
        wc.name = it.key();
        QJsonObject vals = it.value().toObject();
        for (auto vit = vals.begin(); vit != vals.end(); ++vit) {
            wc.values[vit.key()] = vit.value().toDouble();
        }
        result.append(wc);
    }
    return result;
}

QVector<KndFrameBuilder::MacroVar> KndFrameBuilder::parseMacros(const QJsonObject& obj)
{
    QVector<MacroVar> result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        MacroVar mv;
        mv.name = it.key();
        mv.value = it.value().toDouble();
        result.append(mv);
    }
    return result;
}

const QStringList& KndFrameBuilder::plcRegions()
{
    static const QStringList regions = {
        "X", "Y", "F", "G", "R", "S", "K", "D", "TL", "T", "C"
    };
    return regions;
}