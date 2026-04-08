#ifndef MOCKCONFIG_H
#define MOCKCONFIG_H

#include <QObject>
#include <QMap>
#include <QByteArray>

class MockConfig : public QObject
{
    Q_OBJECT
public:
    static MockConfig* instance();
    QByteArray getResponse(quint8 cmd, quint8 sub, quint8 type) const;
    void setResponse(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data);
    void resetToDefault(quint8 cmd, quint8 sub, quint8 type);
    void resetAll();
    void setCoordinateBase(double x, double y, double z);
    void getCoordinateBase(double &x, double &y, double &z) const;
signals:
    void configChanged();
private:
    explicit MockConfig(QObject *parent = nullptr);
    QByteArray buildDefaultCoordinateData(double x, double y, double z);
    QByteArray buildDefaultDeviceInfo();
    QByteArray buildDefaultMachineStatus();

    QMap<QString, QByteArray> m_customResponses;
    double m_coordBaseX, m_coordBaseY, m_coordBaseZ;
};

#endif