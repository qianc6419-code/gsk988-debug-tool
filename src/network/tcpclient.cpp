#include "tcpclient.h"

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_port(0)
{
}

TcpClient::~TcpClient()
{
}

void TcpClient::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_UNUSED(address);
    Q_UNUSED(port);
}

void TcpClient::disconnectFromHost()
{
}

bool TcpClient::isConnected() const
{
    return false;
}

qint64 TcpClient::sendData(const QByteArray &data)
{
    Q_UNUSED(data);
    return -1;
}

void TcpClient::onConnected()
{
}

void TcpClient::onDisconnected()
{
}

void TcpClient::onReadyRead()
{
}

void TcpClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
}
