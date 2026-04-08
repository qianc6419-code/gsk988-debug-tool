#include "mockconfig.h"

MockConfig::MockConfig(QObject *parent)
    : QObject(parent)
    , m_serverPort(8080)
    , m_autoResponse(true)
    , m_responseDelayMs(100)
{
}

MockConfig::~MockConfig()
{
}

MockConfig& MockConfig::instance()
{
    static MockConfig instance;
    return instance;
}

void MockConfig::loadFromFile(const QString &filePath)
{
    Q_UNUSED(filePath);
}

void MockConfig::saveToFile(const QString &filePath)
{
    Q_UNUSED(filePath);
}

void MockConfig::reset()
{
}

QString MockConfig::getServerName() const
{
    return m_serverName;
}

void MockConfig::setServerName(const QString &name)
{
    Q_UNUSED(name);
}

quint16 MockConfig::getServerPort() const
{
    return m_serverPort;
}

void MockConfig::setServerPort(quint16 port)
{
    Q_UNUSED(port);
}

bool MockConfig::isAutoResponseEnabled() const
{
    return m_autoResponse;
}

void MockConfig::setAutoResponseEnabled(bool enabled)
{
    Q_UNUSED(enabled);
}

int MockConfig::getResponseDelayMs() const
{
    return m_responseDelayMs;
}

void MockConfig::setResponseDelayMs(int delayMs)
{
    Q_UNUSED(delayMs);
}

QByteArray MockConfig::getMockResponseData() const
{
    return m_mockResponseData;
}

void MockConfig::setMockResponseData(const QByteArray &data)
{
    Q_UNUSED(data);
}
