#ifndef SIEMENSPROTOCOL_H
#define SIEMENSPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

struct SiemensCommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString dataType; // "String", "Double", "Float", "Int32", "Int16", "Mode", "Status"
    QString desc;
};

class SiemensProtocol : public IProtocol {
    Q_OBJECT
public:
    explicit SiemensProtocol(QObject* parent = nullptr);

    QString name() const override { return "Siemens S7"; }
    QString version() const override { return "1.0"; }

    // 握手（3步）
    QByteArray buildHandshake();
    bool isHandshakeComplete() const;
    int handshakeStep() const { return m_handshakeStep; }
    QByteArray advanceHandshake();

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    static QVector<SiemensCommandDef> allCommandsRaw();
    static SiemensCommandDef commandDefRaw(quint8 cmdCode);
    static QString interpretDataRaw(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);

private:
    int m_handshakeStep;  // 0=未开始, 1=已发step1, 2=已发step2, 3=已发step3, 4=完成(收到step3响应)
    quint8 m_lastRequestCmdCode;
};

#endif // SIEMENSPROTOCOL_H
