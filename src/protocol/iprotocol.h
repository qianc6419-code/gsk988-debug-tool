#ifndef IPROTOCOL_H
#define IPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVector>

struct ProtocolCommand {
    quint8 cmdCode;
    QString name;
    QString category;
    QString paramDesc;
    QString respDesc;
};

struct ParsedResponse {
    quint8 cmdCode = 0;
    bool isValid = false;
    QByteArray rawData;
};

class IProtocol : public QObject {
    Q_OBJECT
public:
    explicit IProtocol(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IProtocol() = default;
    virtual QString name() const = 0;
    virtual QString version() const = 0;

    virtual QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) = 0;
    virtual ParsedResponse parseResponse(const QByteArray& frame) = 0;
    virtual QByteArray extractFrame(QByteArray& buffer) = 0;

    virtual QVector<ProtocolCommand> allCommands() const = 0;
    virtual ProtocolCommand commandDef(quint8 cmdCode) const = 0;

    virtual QString interpretData(quint8 cmdCode, const QByteArray& data) const = 0;

    // Returns a complete response frame ready to send over the transport
    virtual QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const = 0;
};

#endif // IPROTOCOL_H
