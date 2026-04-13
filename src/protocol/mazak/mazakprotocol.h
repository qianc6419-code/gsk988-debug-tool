#ifndef MAZAKPROTOCOL_H
#define MAZAKPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

struct MazakCommandDef {
    quint8  cmdCode;
    QString name;
    QString category;
    QString desc;
};

class MazakProtocol : public IProtocol
{
    Q_OBJECT
public:
    explicit MazakProtocol(QObject* parent = nullptr);

    QString name() const override { return "Mazak Smooth"; }
    QString version() const override { return "1.0"; }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    static const QVector<MazakCommandDef>& commandDefs();

    // Mazak 不走 Transport 层的连接方式
    bool isDllBased() const { return true; }
};

#endif // MAZAKPROTOCOL_H
