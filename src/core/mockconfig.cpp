#include "mockconfig.h"
#include <QDataStream>

MockConfig::MockConfig(QObject *parent)
    : QObject(parent)
    , m_coordBaseX(100.0)
    , m_coordBaseY(40.0)
    , m_coordBaseZ(0.0)
{
}

MockConfig* MockConfig::instance()
{
    static MockConfig inst;
    return &inst;
}

QByteArray MockConfig::getResponse(quint8 cmd, quint8 sub, quint8 type) const
{
    QString key = QString("%1-%2-%3")
        .arg(cmd, 2, 16, QChar('0'))
        .arg(sub, 2, 16, QChar('0'))
        .arg(type, 2, 16, QChar('0'));

    if (m_customResponses.contains(key)) {
        return m_customResponses.value(key);
    }

    switch (cmd) {
    case 0x01:
        return buildDefaultDeviceInfo();
    case 0x02:
        return buildDefaultMachineStatus();
    case 0x03:
        return buildDefaultCoordinateData(m_coordBaseX, m_coordBaseY, m_coordBaseZ);
    case 0x05:
        return QByteArray("\x00", 1);
    default:
        return QByteArray();
    }
}

void MockConfig::setResponse(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data)
{
    QString key = QString("%1-%2-%3")
        .arg(cmd, 2, 16, QChar('0'))
        .arg(sub, 2, 16, QChar('0'))
        .arg(type, 2, 16, QChar('0'));

    m_customResponses.insert(key, data);
    emit configChanged();
}

void MockConfig::resetToDefault(quint8 cmd, quint8 sub, quint8 type)
{
    QString key = QString("%1-%2-%3")
        .arg(cmd, 2, 16, QChar('0'))
        .arg(sub, 2, 16, QChar('0'))
        .arg(type, 2, 16, QChar('0'));

    m_customResponses.remove(key);
    emit configChanged();
}

void MockConfig::resetAll()
{
    m_customResponses.clear();
    emit configChanged();
}

void MockConfig::setCoordinateBase(double x, double y, double z)
{
    m_coordBaseX = x;
    m_coordBaseY = y;
    m_coordBaseZ = z;
}

void MockConfig::getCoordinateBase(double &x, double &y, double &z) const
{
    x = m_coordBaseX;
    y = m_coordBaseY;
    z = m_coordBaseZ;
}

QByteArray MockConfig::buildDefaultDeviceInfo()
{
    QByteArray data;
    data.resize(20);
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // GSK988 padded to 16 bytes + quint16(2025) + quint8(1) + quint8(0)
    QByteArray name = "GSK988";
    name.resize(16, ' ');
    stream.writeRawData(name.constData(), 16);
    stream << quint16(2025);
    stream << quint8(1);
    stream << quint8(0);

    return data;
}

QByteArray MockConfig::buildDefaultMachineStatus()
{
    return QByteArray("\x01\x00\x64\x64\x88\x13\x00\x01", 8);
}

QByteArray MockConfig::buildDefaultCoordinateData(double x, double y, double z)
{
    QByteArray data;
    data.resize(24);  // 6 float32 values
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << float(x);
    stream << float(y);
    stream << float(z);
    stream << float(x + 23.456);
    stream << float(y + 5.789);
    stream << float(z);

    return data;
}