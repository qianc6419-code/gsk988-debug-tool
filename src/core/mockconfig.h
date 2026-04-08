#ifndef MOCKCONFIG_H
#define MOCKCONFIG_H

#include <QObject>
#include <QString>
#include <QMap>

struct MockConfigData
{
    QString serverName;
    quint16 serverPort;
    bool autoResponse;
    int responseDelayMs;
    QByteArray mockResponseData;
};

class MockConfig : public QObject
{
    Q_OBJECT

public:
    static MockConfig& instance();

    void loadFromFile(const QString &filePath);
    void saveToFile(const QString &filePath);
    void reset();

    QString getServerName() const;
    void setServerName(const QString &name);
    quint16 getServerPort() const;
    void setServerPort(quint16 port);
    bool isAutoResponseEnabled() const;
    void setAutoResponseEnabled(bool enabled);
    int getResponseDelayMs() const;
    void setResponseDelayMs(int delayMs);
    QByteArray getMockResponseData() const;
    void setMockResponseData(const QByteArray &data);

signals:
    void configChanged();

private:
    explicit MockConfig(QObject *parent = nullptr);
    ~MockConfig();
    MockConfig(const MockConfig&) = delete;
    MockConfig& operator=(const MockConfig&) = delete;

    QString m_serverName;
    quint16 m_serverPort;
    bool m_autoResponse;
    int m_responseDelayMs;
    QByteArray m_mockResponseData;
};

#endif // MOCKCONFIG_H
