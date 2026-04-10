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
    explicit TcpClient(QObject* parent = nullptr);
    ~TcpClient();

    void connectTo(const QString& ip, quint16 port);
    void disconnect();
    bool isConnected() const;
    void sendFrame(const QByteArray& frame);

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& msg);
    void responseReceived(const QByteArray& frame);
    void responseTimeout();

private slots:
    void onReadyRead();
    void onTimeout();

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;
    QTimer* m_timeoutTimer;
    QByteArray m_pendingFrame;
};

#endif // TCPCLIENT_H
