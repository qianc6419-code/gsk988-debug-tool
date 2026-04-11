#include "modbusframebuilder.h"

QByteArray ModbusFrameBuilder::buildFrame(quint16 txnId, quint8 unitId,
                                           quint8 funcCode, const QByteArray& pduData)
{
    QByteArray frame;
    quint16 length = 1 + 1 + static_cast<quint16>(pduData.size());
    frame.reserve(7 + pduData.size());

    // Transaction ID (big-endian)
    frame.append(static_cast<char>((txnId >> 8) & 0xFF));
    frame.append(static_cast<char>(txnId & 0xFF));

    // Protocol ID (always 0x0000 for Modbus)
    frame.append('\x00');
    frame.append('\x00');

    // Length (big-endian)
    frame.append(static_cast<char>((length >> 8) & 0xFF));
    frame.append(static_cast<char>(length & 0xFF));

    // Unit ID
    frame.append(static_cast<char>(unitId));

    // Function code
    frame.append(static_cast<char>(funcCode));

    // PDU data
    frame.append(pduData);

    return frame;
}

quint16 ModbusFrameBuilder::parseTransactionId(const QByteArray& frame)
{
    if (frame.size() < 2) return 0;
    return (static_cast<quint8>(frame[0]) << 8) | static_cast<quint8>(frame[1]);
}

quint16 ModbusFrameBuilder::parseProtocolId(const QByteArray& frame)
{
    if (frame.size() < 4) return 0;
    return (static_cast<quint8>(frame[2]) << 8) | static_cast<quint8>(frame[3]);
}

quint16 ModbusFrameBuilder::parseLength(const QByteArray& frame)
{
    if (frame.size() < 6) return 0;
    return (static_cast<quint8>(frame[4]) << 8) | static_cast<quint8>(frame[5]);
}

quint8 ModbusFrameBuilder::parseUnitId(const QByteArray& frame)
{
    if (frame.size() < 7) return 0;
    return static_cast<quint8>(frame[6]);
}

quint8 ModbusFrameBuilder::parseFunctionCode(const QByteArray& frame)
{
    if (frame.size() < 8) return 0;
    return static_cast<quint8>(frame[7]);
}

QByteArray ModbusFrameBuilder::encodeAddress(quint16 addr)
{
    QByteArray ba;
    ba.append(static_cast<char>((addr >> 8) & 0xFF));
    ba.append(static_cast<char>(addr & 0xFF));
    return ba;
}

QByteArray ModbusFrameBuilder::encodeQuantity(quint16 qty)
{
    QByteArray ba;
    ba.append(static_cast<char>((qty >> 8) & 0xFF));
    ba.append(static_cast<char>(qty & 0xFF));
    return ba;
}

QByteArray ModbusFrameBuilder::encodeValue(quint16 val)
{
    QByteArray ba;
    ba.append(static_cast<char>((val >> 8) & 0xFF));
    ba.append(static_cast<char>(val & 0xFF));
    return ba;
}

QByteArray ModbusFrameBuilder::encodeCoilValues(const QVector<bool>& coils)
{
    int byteCount = (coils.size() + 7) / 8;
    QByteArray bytes(byteCount, '\0');
    for (int i = 0; i < coils.size(); ++i) {
        if (coils[i])
            bytes[i / 8] |= (1 << (i % 8));
    }
    return bytes;
}

QByteArray ModbusFrameBuilder::encodeRegisterValues(const QVector<quint16>& regs)
{
    QByteArray ba;
    ba.reserve(regs.size() * 2);
    for (quint16 reg : regs) {
        ba.append(static_cast<char>((reg >> 8) & 0xFF));
        ba.append(static_cast<char>(reg & 0xFF));
    }
    return ba;
}

quint16 ModbusFrameBuilder::decodeUInt16(const QByteArray& data, int offset)
{
    if (offset + 2 > data.size()) return 0;
    return (static_cast<quint8>(data[offset]) << 8) | static_cast<quint8>(data[offset + 1]);
}

quint32 ModbusFrameBuilder::decodeUInt32(const QByteArray& data, int offset)
{
    if (offset + 4 > data.size()) return 0;
    return (static_cast<quint32>(static_cast<quint8>(data[offset])) << 24) |
           (static_cast<quint32>(static_cast<quint8>(data[offset + 1])) << 16) |
           (static_cast<quint32>(static_cast<quint8>(data[offset + 2])) << 8) |
           static_cast<quint32>(static_cast<quint8>(data[offset + 3]));
}

QVector<bool> ModbusFrameBuilder::decodeCoils(const QByteArray& data, int offset, int count)
{
    QVector<bool> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        int byteIdx = offset + i / 8;
        int bitIdx = i % 8;
        bool val = false;
        if (byteIdx < data.size())
            val = (static_cast<quint8>(data[byteIdx]) >> bitIdx) & 1;
        result.append(val);
    }
    return result;
}

QVector<quint16> ModbusFrameBuilder::decodeRegisters(const QByteArray& data, int offset, int count)
{
    QVector<quint16> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.append(decodeUInt16(data, offset + i * 2));
    }
    return result;
}

bool ModbusFrameBuilder::isException(quint8 funcCode)
{
    return (funcCode & 0x80) != 0;
}
