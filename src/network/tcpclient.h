#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();
    bool connectTo(const QString &host, quint16 port);
    void disconnect();
    bool isConnected() const { return m_socket && m_socket->state() == QAbstractSocket::ConnectedState; }
    void sendFrame(const QByteArray &frame);
    void setTimeout(int ms) { m_timeout = ms; }
signals:
    void connected();
    void disconnected();
    void readyRead(const QByteArray &data);
    void connectionError(const QString &error);
    void timeout();
private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onWaitTimeout();
private:
    QTcpSocket *m_socket;
    QTimer *m_waitTimer;
    int m_timeout;
};

#endif
