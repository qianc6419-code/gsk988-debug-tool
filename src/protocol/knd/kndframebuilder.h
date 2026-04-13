#ifndef KNDFRAMEBUILDER_H
#define KNDFRAMEBUILDER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

class KndFrameBuilder
{
public:
    // 状态映射: run-status -> 中文描述
    static QString runStatusToString(int status);
    static QString runStatusColor(int status);

    // 模式映射: opr-mode -> 中文描述
    static QString oprModeToString(int mode);

    // 从 JSON 提取坐标信息
    struct AxisCoord {
        QString name;
        double machine = 0.0;
        double absolute = 0.0;
        double relative = 0.0;
        double distanceToGo = 0.0;
    };
    static QVector<AxisCoord> parseCoordinates(const QJsonObject& obj);

    // 从 JSON 提取驱动负载
    struct AxisLoad {
        QString name;
        double load = 0.0;
    };
    static QVector<AxisLoad> parseAxisLoad(const QJsonObject& obj);

    // 从 JSON 提取主轴速率
    struct SpindleSpeed {
        int spNo = 0;
        double setSpeed = 0.0;
        double actSpeed = 0.0;
    };
    static QVector<SpindleSpeed> parseSpindleSpeeds(const QJsonObject& obj);

    // 从 JSON 提取倍率
    static int parseOverride(const QJsonObject& obj);

    // 从 JSON 提取报警
    struct AlarmInfo {
        int no = 0;
        QString message;
    };
    static QVector<AlarmInfo> parseAlarms(const QJsonObject& obj);

    // 从 JSON 提取循环时间
    struct CycleTime {
        int cycleHour = 0;
        int cycleMin = 0;
        int cycleSec = 0;
        int totalHour = 0;
        int totalMin = 0;
        int totalSec = 0;
    };
    static CycleTime parseCycleTime(const QJsonObject& obj);

    // 从 JSON 提取刀具补偿
    struct ToolOffset {
        int ofsNo = 0;
        QMap<QString, double> values; // axis -> value
    };
    static QVector<ToolOffset> parseToolOffsets(const QJsonArray& arr);

    // 从 JSON 提取工件坐标系
    struct WorkCoord {
        QString name;
        QMap<QString, double> values; // axis -> value
    };
    static QVector<WorkCoord> parseWorkCoords(const QJsonObject& obj);

    // 从 JSON 提取宏变量
    struct MacroVar {
        QString name;
        double value = 0.0;
    };
    static QVector<MacroVar> parseMacros(const QJsonObject& obj);

    // PLC 区域列表
    static const QStringList& plcRegions();
};

#endif // KNDFRAMEBUILDER_H