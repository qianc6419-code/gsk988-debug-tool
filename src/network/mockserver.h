#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QByteArray>

class MockServer : public QObject
{
    Q_OBJECT
public:
    explicit MockServer(QObject *parent = nullptr);
    ~MockServer();
    bool start(quint16 port = 8067);
    void stop();
    bool isRunning() const { return m_server.isListening(); }
    void setMockResponse(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data);
    void resetMockResponse(quint8 cmd, quint8 sub, quint8 type);
    void resetAllResponses();
signals:
    void clientConnected(const QString &address);
    void clientDisconnected();
    void mockDataSent(const QByteArray &data);
    void mockDataReceived(const QByteArray &data);
    void serverError(const QString &error);
private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
private:
    QByteArray generateDefaultResponse(quint8 cmd, quint8 sub, quint8 type);
    QByteArray buildMockFrame(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data);
    void initDefaultResponses();

    QTcpServer m_server;
    QTcpSocket *m_clientSocket;
    QMap<QString, QByteArray> m_mockResponses;
    QMap<QString, QByteArray> m_defaultResponses;
    quint16 m_port;
};
#endif
