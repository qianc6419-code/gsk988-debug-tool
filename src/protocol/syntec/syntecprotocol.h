#ifndef SYNTECPROTOCOL_H
#define SYNTECPROTOCOL_H

#include "protocol/iprotocol.h"

class SyntecProtocol : public IProtocol
{
    Q_OBJECT
public:
    explicit SyntecProtocol(QObject* parent = nullptr);

    QString name() const override { return QStringLiteral("Syntec"); }
    QString version() const override { return QStringLiteral("1.0"); }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;
    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    // Handshake
    QByteArray buildHandshake();

    // TID
    quint8 nextTid() { return m_tid++; }

private:
    quint8 m_tid = 0;
    quint8 m_pendingTid = 0;
    quint8 m_pendingCmdCode = 0;
};

#endif // SYNTECPROTOCOL_H
