#include "gsk988protocol.h"
#include <QDebug>

Gsk988Protocol::Gsk988Protocol(QObject *parent)
    : QObject(parent)
{
}

bool Gsk988Protocol::parseResponse(const QByteArray &data, quint8 cmd, QMap<QString, QVariant> &outResult)
{
    Q_UNUSED(cmd);
    Q_UNUSED(outResult);

    if (data.isEmpty()) {
        emit protocolError("Empty response data");
        return false;
    }

    // Dispatch to appropriate parser based on command
    switch (cmd) {
    case 0x01: // Device info
    {
        DeviceInfo info;
        if (parseDeviceInfo(data, info)) {
            outResult["modelName"] = info.modelName;
            outResult["softwareVersion"] = info.softwareVersion;
            outResult["seriesYear"] = info.seriesYear;
            outResult["majorVersion"] = info.majorVersion;
            outResult["minorVersion"] = info.minorVersion;
            return true;
        }
        return false;
    }
    case 0x02: // Machine status
    {
        MachineStatus status;
        if (parseMachineStatus(data, status)) {
            outResult["runState"] = status.runState;
            outResult["mode"] = status.mode;
            outResult["feedOverride"] = status.feedOverride;
            outResult["spindleOverride"] = status.spindleOverride;
            outResult["spindleSpeed"] = status.spindleSpeed;
            outResult["toolNo"] = status.toolNo;
            outResult["coolantOn"] = status.coolantOn;
            return true;
        }
        return false;
    }
    case 0x03: // Coordinate
    {
        CoordinateData coord;
        if (parseCoordinate(data, coord)) {
            outResult["machineX"] = coord.machineX;
            outResult["machineY"] = coord.machineY;
            outResult["machineZ"] = coord.machineZ;
            outResult["workX"] = coord.workX;
            outResult["workY"] = coord.workY;
            outResult["workZ"] = coord.workZ;
            return true;
        }
        return false;
    }
    case 0x04: // Alarm info
    {
        QVector<AlarmInfo> alarms;
        if (parseAlarmInfo(data, alarms)) {
            QVariantList alarmList;
            for (const AlarmInfo &alarm : alarms) {
                QMap<QString, QVariant> alarmMap;
                alarmMap["alarmNo"] = alarm.alarmNo;
                alarmMap["alarmText"] = alarm.alarmText;
                alarmMap["alarmTime"] = alarm.alarmTime;
                alarmList.append(alarmMap);
            }
            outResult["alarms"] = alarmList;
            return true;
        }
        return false;
    }
    default:
        emit protocolError(QString("Unknown command: 0x%1").arg(cmd, 2, 16, QChar('0')));
        return false;
    }
}

bool Gsk988Protocol::parseDeviceInfo(const QByteArray &data, DeviceInfo &info)
{
    // Requires at least 20 bytes: 16 for model name + 4 for version info
    if (data.size() < 20) {
        emit protocolError("Device info data too short");
        return false;
    }

    // Read 16-byte model name
    info.modelName = QString::fromLatin1(data.mid(0, 16)).trimmed();

    // Read version info at offset 16
    // byte0: year high byte, byte1: year low byte
    // byte2: major version, byte3: minor version
    quint8 yearHi = static_cast<quint8>(data[16]);
    quint8 yearLo = static_cast<quint8>(data[17]);
    info.seriesYear = (static_cast<quint16>(yearHi) << 8) | yearLo;
    info.majorVersion = static_cast<quint8>(data[18]);
    info.minorVersion = static_cast<quint8>(data[19]);

    // Build software version string
    info.softwareVersion = QString("%1.%2")
        .arg(info.majorVersion)
        .arg(info.minorVersion);

    return true;
}

bool Gsk988Protocol::parseMachineStatus(const QByteArray &data, MachineStatus &status)
{
    // Requires exactly 8 bytes
    if (data.size() < 8) {
        emit protocolError("Machine status data too short");
        return false;
    }

    const quint8 *bytes = reinterpret_cast<const quint8*>(data.data());

    // byte0: runState (lower 3 bits)
    status.runState = bytes[0] & 0x07;

    // byte1: mode (lower 3 bits)
    status.mode = bytes[1] & 0x07;

    // byte2: feedOverride (0-255 -> 0-100%)
    status.feedOverride = (static_cast<quint16>(bytes[2]) * 100) / 255;

    // byte3: spindleOverride
    status.spindleOverride = bytes[3];

    // byte4-5: spindleSpeed (Little Endian)
    status.spindleSpeed = static_cast<quint16>(bytes[4]) | (static_cast<quint16>(bytes[5]) << 8);

    // byte6: toolNo (lower 5 bits)
    status.toolNo = bytes[6] & 0x1F;

    // byte7: coolantOn (lowest bit)
    status.coolantOn = bytes[7] & 0x01;

    return true;
}

bool Gsk988Protocol::parseCoordinate(const QByteArray &data, CoordinateData &coord)
{
    // Requires 24 bytes: 6 float32 values (4 bytes each)
    if (data.size() < 24) {
        emit protocolError("Coordinate data too short");
        return false;
    }

    const quint8 *bytes = reinterpret_cast<const quint8*>(data.data());

    // machineX/Y/Z (first 3 float32 values)
    coord.machineX = decodeFloat32(bytes);
    coord.machineY = decodeFloat32(bytes + 4);
    coord.machineZ = decodeFloat32(bytes + 8);

    // workX/Y/Z (next 3 float32 values)
    coord.workX = decodeFloat32(bytes + 12);
    coord.workY = decodeFloat32(bytes + 16);
    coord.workZ = decodeFloat32(bytes + 20);

    return true;
}

bool Gsk988Protocol::parseAlarmInfo(const QByteArray &data, QVector<AlarmInfo> &alarms)
{
    if (data.isEmpty()) {
        emit protocolError("Alarm info data empty");
        return false;
    }

    const quint8 *bytes = reinterpret_cast<const quint8*>(data.data());
    quint8 count = bytes[0];

    alarms.clear();
    int offset = 1;

    for (quint8 i = 0; i < count; ++i) {
        if (offset >= data.size()) {
            break;
        }

        AlarmInfo alarm;
        alarm.alarmNo = bytes[offset];

        if (offset + 1 + 16 > data.size()) {
            break;
        }

        alarm.alarmText = QString::fromLatin1(data.mid(offset + 1, 16)).trimmed();
        alarm.alarmTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        alarms.append(alarm);
        offset += 1 + 16; // alarmNo + 16 bytes text
    }

    return true;
}

QString Gsk988Protocol::runStateToString(quint8 state)
{
    switch (state) {
    case 0: return QString::fromUtf8("停止");
    case 1: return QString::fromUtf8("运行中");
    case 2: return QString::fromUtf8("暂停");
    case 3: return QString::fromUtf8("急停");
    default: return QString::fromUtf8("未知");
    }
}

QString Gsk988Protocol::modeToString(quint8 mode)
{
    switch (mode) {
    case 0: return "AUTO";
    case 1: return "MDI";
    case 2: return "MANUAL";
    case 3: return "JOG";
    case 4: return "HANDLE";
    default: return "UNKNOWN";
    }
}

double Gsk988Protocol::decodeFloat32(const quint8 *bytes) const
{
    // Read 4 bytes as Little Endian uint32
    quint32 bits = static_cast<quint32>(bytes[0])
                 | (static_cast<quint32>(bytes[1]) << 8)
                 | (static_cast<quint32>(bytes[2]) << 16)
                 | (static_cast<quint32>(bytes[3]) << 24);

    // Reinterpret as float (IEEE 754)
    float value;
    memcpy(&value, &bits, sizeof(float));
    return static_cast<double>(value);
}
