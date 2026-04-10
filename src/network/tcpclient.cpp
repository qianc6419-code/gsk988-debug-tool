#include "tcpclient.h"
#include "protocol/framebuilder.h"

TcpClient::TcpClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(3000);

    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        emit connected();
    });

    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        m_timeoutTimer->stop();
        m_buffer.clear();
        m_pendingFrame.clear();
        emit disconnected();
    });

    using ErrorSignal = void (QTcpSocket::*)(QAbstractSocket::SocketError);
    connect(m_socket, static_cast<ErrorSignal>(&QTcpSocket::errorOccurred), this,
            [this](QAbstractSocket::SocketError) {
        emit connectionError(m_socket->errorString());
    });

    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_timeoutTimer, &QTimer::timeout, this, &TcpClient::onTimeout);
}

TcpClient::~TcpClient()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();
}

void TcpClient::connectTo(const QString& ip, quint16 port)
{
    m_buffer.clear();
    m_pendingFrame.clear();
    m_socket->connectToHost(ip, port);
}

void TcpClient::disconnect()
{
    m_timeoutTimer->stop();
    m_socket->disconnectFromHost();
}

bool TcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::sendFrame(const QByteArray& frame)
{
    if (!isConnected())
        return;

    m_pendingFrame = frame;
    m_socket->write(frame);
    m_timeoutTimer->start();
}

void TcpClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    // Extract all complete frames
    QByteArray frame;
    while (!(frame = FrameBuilder::extractFrame(m_buffer)).isEmpty()) {
        m_timeoutTimer->stop();
        emit responseReceived(frame);
    }
}

void TcpClient::onTimeout()
{
    emit responseTimeout();
}
