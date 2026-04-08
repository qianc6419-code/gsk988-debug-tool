#include "tcpclient.h"
#include <QTimer>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_waitTimer(nullptr)
    , m_timeout(5000)
{
    m_socket = new QTcpSocket(this);
    m_waitTimer = new QTimer(this);
    m_waitTimer->setSingleShot(true);

    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &TcpClient::onError);
    connect(m_waitTimer, &QTimer::timeout, this, &TcpClient::onWaitTimeout);
}

TcpClient::~TcpClient()
{
    disconnect();
}

bool TcpClient::connectTo(const QString &host, quint16 port)
{
    if (isConnected()) {
        disconnect();
    }
    m_socket->connectToHost(host, port);
    if (m_socket->waitForConnected(m_timeout)) {
        emit connected();
        return true;
    } else {
        emit connectionError(m_socket->errorString());
        return false;
    }
}

void TcpClient::disconnect()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

void TcpClient::sendFrame(const QByteArray &frame)
{
    if (!isConnected()) {
        emit connectionError("未连接");
        return;
    }
    m_socket->write(frame);
    m_socket->flush();
    m_waitTimer->start(m_timeout);
}

void TcpClient::onReadyRead()
{
    m_waitTimer->stop();
    QByteArray data = m_socket->readAll();
    emit readyRead(data);
}

void TcpClient::onDisconnected()
{
    emit disconnected();
}

void TcpClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit connectionError(m_socket->errorString());
}

void TcpClient::onWaitTimeout()
{
    emit timeout();
}
