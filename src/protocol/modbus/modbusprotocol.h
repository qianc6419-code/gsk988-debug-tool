#ifndef MODBUSPROTOCOL_H
#define MODBUSPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

struct ModbusCommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

class ModbusProtocol : public IProtocol {
    Q_OBJECT
public:
    explicit ModbusProtocol(QObject* parent = nullptr);

    QString name() const override { return "Modbus TCP"; }
    QString version() const override { return "1.0"; }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    // Internal: raw command table
    static QVector<ModbusCommandDef> allCommandsRaw();
    static ModbusCommandDef commandDefRaw(quint8 cmdCode);
    static QString interpretDataRaw(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);

private:
    mutable quint16 m_transactionId = 0;
    quint16 nextTransactionId() const;

    // Mock memory for mock server
    static QVector<bool> s_mockCoils;
    static QVector<bool> s_mockDiscreteInputs;
    static QVector<quint16> s_mockHoldingRegs;
    static QVector<quint16> s_mockInputRegs;
    static void initMockMemory();

    static QVector<ModbusCommandDef> s_commands;
};

#endif // MODBUSPROTOCOL_H
