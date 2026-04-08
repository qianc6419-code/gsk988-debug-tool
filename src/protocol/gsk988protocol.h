#ifndef GSK988PROTOCOL_H
#define GSK988PROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QQueue>

struct Gsk988Frame
{
    quint8 header;
    quint8 dataLength;
    QByteArray data;
    quint16 crc;
};

class Gsk988Protocol : public QObject
{
    Q_OBJECT

public:
    explicit Gsk988Protocol(QObject *parent = nullptr);
    ~Gsk988Protocol();

    QByteArray buildFrame(quint8 command, const QByteArray &payload);
    bool parseFrame(const QByteArray &rawData);
    bool validateCrc(const Gsk988Frame &frame);

signals:
    void frameReceived(const Gsk988Frame &frame);
    void frameSent(const Gsk988Frame &frame);
    void protocolError(const QString &errorString);

private:
    QQueue<Gsk988Frame> m_frameQueue;
};

#endif // GSK988PROTOCOL_H
