#ifndef MODBUSFRAMEBUILDER_H
#define MODBUSFRAMEBUILDER_H

#include <QByteArray>
#include <QVector>

class ModbusFrameBuilder
{
public:
    // Build a complete Modbus TCP ADU frame
    static QByteArray buildFrame(quint16 txnId, quint8 unitId,
                                 quint8 funcCode, const QByteArray& pduData);

    // MBAP header field extraction
    static quint16 parseTransactionId(const QByteArray& frame);
    static quint16 parseProtocolId(const QByteArray& frame);
    static quint16 parseLength(const QByteArray& frame);
    static quint8  parseUnitId(const QByteArray& frame);
    static quint8  parseFunctionCode(const QByteArray& frame);

    // Big-endian parameter encoding
    static QByteArray encodeAddress(quint16 addr);
    static QByteArray encodeQuantity(quint16 qty);
    static QByteArray encodeValue(quint16 val);
    static QByteArray encodeCoilValues(const QVector<bool>& coils);
    static QByteArray encodeRegisterValues(const QVector<quint16>& regs);

    // Big-endian parameter decoding
    static quint16 decodeUInt16(const QByteArray& data, int offset);
    static quint32 decodeUInt32(const QByteArray& data, int offset);
    static QVector<bool>    decodeCoils(const QByteArray& data, int offset, int count);
    static QVector<quint16> decodeRegisters(const QByteArray& data, int offset, int count);

    // Check if function code indicates an exception response
    static bool isException(quint8 funcCode);
};

#endif // MODBUSFRAMEBUILDER_H
