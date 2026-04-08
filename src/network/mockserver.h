#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class MockServer : public QObject
{
    Q_OBJECT

public:
    explicit MockServer(QObject *parent = nullptr);
    ~MockServer();

    bool start(quint16 port);
    void stop();
    bool isRunning() const;
    void setResponseData(const QByteArray &data);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString &clientInfo);
    void clientDisconnected(const QString &clientInfo);
    void dataReceivedFromClient(const QByteArray &data);
    void errorOccurred(const QString &errorString);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    QTcpServer *m_server;
    QList<QTcpSocket *> m_clients;
    QByteArray m_responseData;
    quint16 m_port;
};

#endif // MOCKSERVER_H
