#include "mockserver.h"
#include "protocol/gsk988protocol.h"
#include "protocol/framebuilder.h"

MockServer::MockServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_protocol(new Gsk988Protocol(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &MockServer::onNewConnection);
}

MockServer::~MockServer()
{
    stop();
}

bool MockServer::start(quint16 port)
{
    if (m_server->isListening())
        return true;

    bool ok = m_server->listen(QHostAddress::LocalHost, port);
    if (ok) {
        emit logMessage(QString("Mock服务器已启动，端口: %1").arg(port));
    } else {
        emit logMessage(QString("Mock服务器启动失败: %1").arg(m_server->errorString()));
    }
    return ok;
}

void MockServer::stop()
{
    for (auto* client : m_clients) {
        client->abort();
        client->deleteLater();
    }
    m_clients.clear();
    m_server->close();
}

bool MockServer::isRunning() const
{
    return m_server->isListening();
}

quint16 MockServer::serverPort() const
{
    return m_server->serverPort();
}

void MockServer::onNewConnection()
{
    QTcpSocket* client = m_server->nextPendingConnection();
    m_clients.append(client);

    connect(client, &QTcpSocket::readyRead, this, &MockServer::onClientReadyRead);
    connect(client, &QTcpSocket::disconnected, this, [this, client]() {
        m_clients.removeOne(client);
        client->deleteLater();
    });

    emit logMessage(QString("Mock客户端已连接: %1:%2")
                        .arg(client->peerAddress().toString())
                        .arg(client->peerPort()));
}

void MockServer::onClientReadyRead()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray buffer = client->readAll();

    // Extract and process complete frames
    QByteArray frame;
    while (!(frame = FrameBuilder::extractFrame(buffer)).isEmpty()) {
        // Parse request: skip head(2) + length(2) + request_id(8) = 12
        if (frame.size() < 13) continue;

        quint8 cmdCode = static_cast<quint8>(frame[12]);
        QByteArray requestData = frame.mid(13);

        emit logMessage(QString("Mock收到请求: 命令码 0x%1")
                            .arg(cmdCode, 2, 16, QChar('0')).toUpper());

        // Generate mock response
        QByteArray respData = Gsk988Protocol::mockResponseData(cmdCode, requestData);
        QByteArray respFrame = FrameBuilder::buildResponseFrame(respData);

        client->write(respFrame);
        client->flush();

        emit logMessage(QString("Mock发送响应: 命令码 0x%1, 数据长度 %2字节")
                            .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                            .arg(respData.size()));
    }
}
