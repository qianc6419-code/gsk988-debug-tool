#include "modbusframebuilder.h"
#include <cstring>

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
            bytes[i / 8] = static_cast<char>(static_cast<quint8>(bytes[i / 8]) | (1 << (i % 8)));
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

// --- Byte order aware decode helpers ---

namespace {

// Reorder bytes for 16-bit values according to ByteOrder
// Returns 2 reordered bytes as [high, low] for the value reconstruction.
void reorder2(const QByteArray& data, int offset,
              ModbusFrameBuilder::ByteOrder order,
              quint8& b0, quint8& b1)
{
    quint8 raw0 = static_cast<quint8>(data[offset]);
    quint8 raw1 = static_cast<quint8>(data[offset + 1]);
    switch (order) {
    case ModbusFrameBuilder::ABCD: // big-endian
    case ModbusFrameBuilder::CDAB: // word swap — only one word, same as ABCD
        b0 = raw0; b1 = raw1; break;
    case ModbusFrameBuilder::BADC: // byte swap within word
    case ModbusFrameBuilder::DCBA: // full reverse — for 16-bit same as BADC
        b0 = raw1; b1 = raw0; break;
    }
}

// Reorder bytes for 32-bit values (4 bytes)
void reorder4(const QByteArray& data, int offset,
              ModbusFrameBuilder::ByteOrder order,
              quint8& b0, quint8& b1, quint8& b2, quint8& b3)
{
    quint8 raw0 = static_cast<quint8>(data[offset]);
    quint8 raw1 = static_cast<quint8>(data[offset + 1]);
    quint8 raw2 = static_cast<quint8>(data[offset + 2]);
    quint8 raw3 = static_cast<quint8>(data[offset + 3]);
    switch (order) {
    case ModbusFrameBuilder::ABCD:
        b0 = raw0; b1 = raw1; b2 = raw2; b3 = raw3; break;
    case ModbusFrameBuilder::BADC:
        b0 = raw1; b1 = raw0; b2 = raw3; b3 = raw2; break;
    case ModbusFrameBuilder::CDAB:
        b0 = raw2; b1 = raw3; b2 = raw0; b3 = raw1; break;
    case ModbusFrameBuilder::DCBA:
        b0 = raw3; b1 = raw2; b2 = raw1; b3 = raw0; break;
    }
}

// Reorder bytes for 64-bit values (8 bytes)
void reorder8(const QByteArray& data, int offset,
              ModbusFrameBuilder::ByteOrder order,
              quint8 out[8])
{
    quint8 raw[8];
    for (int i = 0; i < 8; ++i)
        raw[i] = static_cast<quint8>(data[offset + i]);

    switch (order) {
    case ModbusFrameBuilder::ABCD:
        for (int i = 0; i < 8; ++i) out[i] = raw[i];
        break;
    case ModbusFrameBuilder::BADC:
        // swap bytes within each 16-bit word
        out[0] = raw[1]; out[1] = raw[0];
        out[2] = raw[3]; out[3] = raw[2];
        out[4] = raw[5]; out[5] = raw[4];
        out[6] = raw[7]; out[7] = raw[6];
        break;
    case ModbusFrameBuilder::CDAB:
        // swap 16-bit word pairs: bytes 0-1 <-> 4-5, 2-3 <-> 6-7
        out[0] = raw[4]; out[1] = raw[5];
        out[2] = raw[6]; out[3] = raw[7];
        out[4] = raw[0]; out[5] = raw[1];
        out[6] = raw[2]; out[7] = raw[3];
        break;
    case ModbusFrameBuilder::DCBA:
        // reverse all bytes
        for (int i = 0; i < 8; ++i) out[i] = raw[7 - i];
        break;
    }
}

} // anonymous namespace

qint8 ModbusFrameBuilder::decodeInt8(const QByteArray& data, int offset)
{
    if (offset >= data.size()) return 0;
    return static_cast<qint8>(static_cast<quint8>(data[offset]));
}

quint8 ModbusFrameBuilder::decodeUInt8(const QByteArray& data, int offset)
{
    if (offset >= data.size()) return 0;
    return static_cast<quint8>(data[offset]);
}

qint16 ModbusFrameBuilder::decodeInt16(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 2 > data.size()) return 0;
    quint8 b0, b1;
    reorder2(data, offset, order, b0, b1);
    return static_cast<qint16>(static_cast<quint16>((b0 << 8) | b1));
}

quint16 ModbusFrameBuilder::decodeUInt16BO(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 2 > data.size()) return 0;
    quint8 b0, b1;
    reorder2(data, offset, order, b0, b1);
    return (static_cast<quint16>(b0) << 8) | static_cast<quint16>(b1);
}

qint32 ModbusFrameBuilder::decodeInt32(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 4 > data.size()) return 0;
    quint8 b0, b1, b2, b3;
    reorder4(data, offset, order, b0, b1, b2, b3);
    return static_cast<qint32>(
        (static_cast<quint32>(b0) << 24) |
        (static_cast<quint32>(b1) << 16) |
        (static_cast<quint32>(b2) << 8)  |
         static_cast<quint32>(b3));
}

quint32 ModbusFrameBuilder::decodeUInt32BO(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 4 > data.size()) return 0;
    quint8 b0, b1, b2, b3;
    reorder4(data, offset, order, b0, b1, b2, b3);
    return (static_cast<quint32>(b0) << 24) |
           (static_cast<quint32>(b1) << 16) |
           (static_cast<quint32>(b2) << 8)  |
            static_cast<quint32>(b3);
}

qint64 ModbusFrameBuilder::decodeInt64(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 8 > data.size()) return 0;
    quint8 b[8];
    reorder8(data, offset, order, b);
    quint64 val = 0;
    for (int i = 0; i < 8; ++i)
        val = (val << 8) | static_cast<quint64>(b[i]);
    return static_cast<qint64>(val);
}

quint64 ModbusFrameBuilder::decodeUInt64(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 8 > data.size()) return 0;
    quint8 b[8];
    reorder8(data, offset, order, b);
    quint64 val = 0;
    for (int i = 0; i < 8; ++i)
        val = (val << 8) | static_cast<quint64>(b[i]);
    return val;
}

float ModbusFrameBuilder::decodeFloat32(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 4 > data.size()) return 0.0f;
    quint8 b0, b1, b2, b3;
    reorder4(data, offset, order, b0, b1, b2, b3);
    quint32 raw = (static_cast<quint32>(b0) << 24) |
                  (static_cast<quint32>(b1) << 16) |
                  (static_cast<quint32>(b2) << 8)  |
                   static_cast<quint32>(b3);
    float result;
    std::memcpy(&result, &raw, sizeof(float));
    return result;
}

double ModbusFrameBuilder::decodeDouble(const QByteArray& data, int offset, ByteOrder order)
{
    if (offset + 8 > data.size()) return 0.0;
    quint8 b[8];
    reorder8(data, offset, order, b);
    quint64 raw = 0;
    for (int i = 0; i < 8; ++i)
        raw = (raw << 8) | static_cast<quint64>(b[i]);
    double result;
    std::memcpy(&result, &raw, sizeof(double));
    return result;
}
