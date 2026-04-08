#include "mockserver.h"

MockServer::MockServer(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_port(0)
{
}

MockServer::~MockServer()
{
}

bool MockServer::start(quint16 port)
{
    Q_UNUSED(port);
    return false;
}

void MockServer::stop()
{
}

bool MockServer::isRunning() const
{
    return false;
}

void MockServer::setResponseData(const QByteArray &data)
{
    Q_UNUSED(data);
}

void MockServer::onNewConnection()
{
}

void MockServer::onClientDisconnected()
{
}

void MockServer::onReadyRead()
{
}
