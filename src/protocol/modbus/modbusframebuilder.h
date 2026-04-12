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

    // Byte order aware decoding
    enum ByteOrder { ABCD, BADC, CDAB, DCBA };

    static qint16  decodeInt16(const QByteArray& data, int offset, ByteOrder order);
    static quint16 decodeUInt16BO(const QByteArray& data, int offset, ByteOrder order);
    static qint32  decodeInt32(const QByteArray& data, int offset, ByteOrder order);
    static quint32 decodeUInt32BO(const QByteArray& data, int offset, ByteOrder order);
    static qint64  decodeInt64(const QByteArray& data, int offset, ByteOrder order);
    static quint64 decodeUInt64(const QByteArray& data, int offset, ByteOrder order);
    static float   decodeFloat32(const QByteArray& data, int offset, ByteOrder order);
    static double  decodeDouble(const QByteArray& data, int offset, ByteOrder order);
    static qint8   decodeInt8(const QByteArray& data, int offset);
    static quint8  decodeUInt8(const QByteArray& data, int offset);

    // Check if function code indicates an exception response
    static bool isException(quint8 funcCode);
};

#endif // MODBUSFRAMEBUILDER_H
