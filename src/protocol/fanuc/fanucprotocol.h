#ifndef FANUCPROTOCOL_H
#define FANUCPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

struct FanucCommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

class FanucProtocol : public IProtocol {
    Q_OBJECT
public:
    explicit FanucProtocol(QObject* parent = nullptr);

    QString name() const override { return "Fanuc FOCAS"; }
    QString version() const override { return "1.0"; }

    QByteArray buildHandshake();

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    static QVector<FanucCommandDef> allCommandsRaw();
    static FanucCommandDef commandDefRaw(quint8 cmdCode);
    static QString interpretDataRaw(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);

private:
    bool m_handshakeDone;
    quint8 m_lastRequestCmdCode;  // stored by buildRequest/buildHandshake, used by mock server
};

#endif // FANUCPROTOCOL_H
