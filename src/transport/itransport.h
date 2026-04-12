#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include <QObject>
#include <QByteArray>

class QWidget;

class ITransport : public QObject {
    Q_OBJECT
public:
    explicit ITransport(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITransport() = default;
    virtual QString name() const = 0;
    virtual QWidget* createConfigWidget() = 0;
    virtual void connectToHost() = 0;
    virtual void disconnectFromHost() = 0;
    virtual bool isConnected() const = 0;
    virtual void send(const QByteArray& data) = 0;

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& msg);
};

#endif // ITRANSPORT_H
