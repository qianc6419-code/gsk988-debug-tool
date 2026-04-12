#include "mockserver.h"
#include "protocol/iprotocol.h"

MockServer::MockServer(IProtocol* protocol, QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_protocol(protocol)
{
    connect(m_server, &QTcpServer::newConnection, this, &MockServer::onNewConnection);
}

MockServer::~MockServer()
{
    stop();
}

bool MockServer::start(quint16 port)
{
    if (m_server->isListening()) return true;
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

bool MockServer::isRunning() const { return m_server->isListening(); }
quint16 MockServer::serverPort() const { return m_server->serverPort(); }

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

    QByteArray frame;
    while (!(frame = m_protocol->extractFrame(buffer)).isEmpty()) {
        ParsedResponse resp = m_protocol->parseResponse(frame);

        QByteArray respFrame = m_protocol->mockResponse(resp.cmdCode, resp.rawData);
        client->write(respFrame);
        client->flush();

        emit logMessage(QString("Mock响应: 命令码 0x%1")
                            .arg(resp.cmdCode, 2, 16, QChar('0')).toUpper());
    }
}
