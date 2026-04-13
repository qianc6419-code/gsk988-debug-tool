#ifndef KNDPROTOCOL_H
#define KNDPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

class KndProtocol : public IProtocol
{
    Q_OBJECT
public:
    explicit KndProtocol(QObject* parent = nullptr);

    QString name() const override { return "KND REST API"; }
    QString version() const override { return "1.2"; }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    bool isHttpBased() const { return true; }

    static const QVector<ProtocolCommand>& staticCommands();
};

#endif // KNDPROTOCOL_H