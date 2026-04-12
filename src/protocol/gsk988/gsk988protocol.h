#ifndef GSK988PROTOCOL_PLUGIN_H
#define GSK988PROTOCOL_PLUGIN_H

#include "protocol/iprotocol.h"
#include <QVector>

class Gsk988FrameBuilder;

struct Gsk988CommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

class Gsk988Protocol : public IProtocol {
    Q_OBJECT
public:
    explicit Gsk988Protocol(QObject* parent = nullptr);

    QString name() const override { return "GSK988"; }
    QString version() const override { return "2.0"; }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    // Internal: raw command table
    static QVector<Gsk988CommandDef> allCommandsRaw();
    static Gsk988CommandDef commandDefRaw(quint8 cmdCode);
    static QString interpretDataRaw(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);
};

#endif // GSK988PROTOCOL_PLUGIN_H
