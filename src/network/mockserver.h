#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class Gsk988Protocol;

class MockServer : public QObject
{
    Q_OBJECT
public:
    explicit MockServer(QObject* parent = nullptr);
    ~MockServer();

    bool start(quint16 port = 6000);
    void stop();
    bool isRunning() const;
    quint16 serverPort() const;

signals:
    void logMessage(const QString& msg);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
    Gsk988Protocol* m_protocol;
};

#endif // MOCKSERVER_H
