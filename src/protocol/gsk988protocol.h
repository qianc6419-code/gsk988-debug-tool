#ifndef GSK988PROTOCOL_H
#define GSK988PROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QVector>

struct DeviceInfo {
    QString modelName;
    QString softwareVersion;
    quint16 seriesYear;
    quint8 majorVersion;
    quint8 minorVersion;
};

struct MachineStatus {
    quint8 runState;  // 0=stop, 1=running, 2=pause, 3=estop
    quint8 mode;      // 0=AUTO, 1=MDI, 2=MANUAL, 3=JOG, 4=HANDLE
    quint16 feedOverride;
    quint16 spindleOverride;
    quint16 spindleSpeed;
    quint8 toolNo;
    quint8 coolantOn;
};

struct CoordinateData {
    double machineX, machineY, machineZ;
    double workX, workY, workZ;
};

struct AlarmInfo {
    quint8 alarmNo;
    QString alarmText;
    QString alarmTime;
};

class Gsk988Protocol : public QObject
{
    Q_OBJECT
public:
    explicit Gsk988Protocol(QObject *parent = nullptr);
    bool parseResponse(const QByteArray &data, quint8 cmd, QMap<QString, QVariant> &outResult);
    bool parseDeviceInfo(const QByteArray &data, DeviceInfo &info);
    bool parseMachineStatus(const QByteArray &data, MachineStatus &status);
    bool parseCoordinate(const QByteArray &data, CoordinateData &coord);
    bool parseAlarmInfo(const QByteArray &data, QVector<AlarmInfo> &alarms);
    static QString runStateToString(quint8 state);
    static QString modeToString(quint8 mode);
signals:
    void protocolError(const QString &error);
private:
    double decodeFloat32(const quint8 *bytes) const;
};

#endif
