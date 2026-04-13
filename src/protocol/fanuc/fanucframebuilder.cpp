#include "fanucframebuilder.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// Static constants
const QByteArray FanucFrameBuilder::FUNC_READ    = QByteArray("\x00\x01\x21\x01", 4);
const QByteArray FanucFrameBuilder::FUNC_RESPONSE = QByteArray("\x00\x01\x21\x02", 4);

// ---------------------------------------------------------------------------
// Frame building
// ---------------------------------------------------------------------------

QByteArray FanucFrameBuilder::buildHandshake()
{
    QByteArray frame;
    frame.reserve(12);
    // Header: 4 x 0xA0
    frame.append(4, static_cast<char>(HEADER_BYTE));
    // Function code: 00 01 01 01
    frame.append('\x00');
    frame.append('\x01');
    frame.append('\x01');
    frame.append('\x01');
    // Data length: 00 02
    frame.append('\x00');
    frame.append('\x02');
    // Block count: 00 02
    frame.append('\x00');
    frame.append('\x02');
    return frame;
}

QByteArray FanucFrameBuilder::buildSingleBlockRequest(quint16 ncFlag,
                                                        quint16 blockFuncCode,
                                                        const QByteArray& blockParams)
{
    QByteArray block = buildBlock(ncFlag, blockFuncCode, blockParams);
    QVector<QByteArray> blocks;
    blocks.append(block);
    return buildMultiBlockRequest(blocks);
}

QByteArray FanucFrameBuilder::buildMultiBlockRequest(const QVector<QByteArray>& blocks)
{
    QByteArray frame;
    int dataLen = 2; // block count field
    for (const QByteArray& blk : blocks)
        dataLen += blk.size();

    frame.reserve(10 + dataLen);

    // Header: 4 x 0xA0
    frame.append(4, static_cast<char>(HEADER_BYTE));
    // Function code: FUNC_READ
    frame.append(FUNC_READ);
    // Data length (big-endian)
    frame.append(static_cast<char>((dataLen >> 8) & 0xFF));
    frame.append(static_cast<char>(dataLen & 0xFF));
    // Block count (big-endian)
    frame.append(static_cast<char>((blocks.size() >> 8) & 0xFF));
    frame.append(static_cast<char>(blocks.size() & 0xFF));
    // Append all blocks
    for (const QByteArray& blk : blocks)
        frame.append(blk);

    return frame;
}

QByteArray FanucFrameBuilder::buildResponse(const QByteArray& dataPayload)
{
    QByteArray frame;
    int dataLen = dataPayload.size();

    frame.reserve(10 + dataLen);

    // Header: 4 x 0xA0
    frame.append(4, static_cast<char>(HEADER_BYTE));
    // Function code: FUNC_RESPONSE
    frame.append(FUNC_RESPONSE);
    // Data length (big-endian)
    frame.append(static_cast<char>((dataLen >> 8) & 0xFF));
    frame.append(static_cast<char>(dataLen & 0xFF));
    // Payload
    frame.append(dataPayload);

    return frame;
}

// ---------------------------------------------------------------------------
// Block building
// ---------------------------------------------------------------------------

QByteArray FanucFrameBuilder::buildBlock(quint16 ncFlag, quint16 blockFuncCode,
                                           const QByteArray& params20)
{
    QByteArray block;
    block.reserve(28);

    // Block length: 0x001C (28 bytes), big-endian
    block.append('\x00');
    block.append('\x1C');
    // NC/PMC flag (big-endian)
    block.append(static_cast<char>((ncFlag >> 8) & 0xFF));
    block.append(static_cast<char>(ncFlag & 0xFF));
    // Reserved: 0x0001
    block.append('\x00');
    block.append('\x01');
    // Block function code (big-endian)
    block.append(static_cast<char>((blockFuncCode >> 8) & 0xFF));
    block.append(static_cast<char>(blockFuncCode & 0xFF));
    // Params: exactly 20 bytes, padded with zeros
    QByteArray padded = params20.leftJustified(20, '\0', true);
    block.append(padded);

    return block;
}

QByteArray FanucFrameBuilder::buildReadPMCBlock(char addrType, int startAddr, int endAddr,
                                                  int pmcTypeNum, int dataType)
{
    quint8 typeNum = pmcAddrTypeToNum(addrType);

    QByteArray params;
    params.reserve(20);
    // startAddr (4B big-endian)
    params.append(encodeInt32(startAddr));
    // endAddr (4B big-endian)
    params.append(encodeInt32(endAddr));
    // typeNum (4B big-endian)
    params.append(encodeInt32(typeNum));
    // dataType (4B big-endian)
    params.append(encodeInt32(dataType));
    // padding (4B)
    params.append(4, '\0');

    // PMC read: ncFlag=0x0002 (PMC), blockFuncCode=0x8001
    return buildBlock(0x0002, 0x8001, params);
}

QByteArray FanucFrameBuilder::buildWritePMCBlockByte(char addrType, int startAddr, int endAddr,
                                                       int pmcTypeNum, quint8 value)
{
    quint8 typeNum = pmcAddrTypeToNum(addrType);

    QByteArray params;
    params.reserve(20);
    // startAddr (4B)
    params.append(encodeInt32(startAddr));
    // endAddr (4B)
    params.append(encodeInt32(endAddr));
    // typeNum (4B)
    params.append(encodeInt32(typeNum));
    // dataType = 0x00 (byte) (4B)
    params.append(encodeInt32(0x00));
    // dataLen = 1 (4B)
    params.append(encodeInt32(0x00000001));

    // Build the 28B header block, then append value
    QByteArray block = buildBlock(0x0002, 0x8002, params);
    block.resize(28); // trim any excess from leftJustified, ensure exactly 28
    block.append(static_cast<char>(value));

    return block;
}

QByteArray FanucFrameBuilder::buildWritePMCBlockWord(char addrType, int startAddr, int endAddr,
                                                       int pmcTypeNum, qint16 value)
{
    quint8 typeNum = pmcAddrTypeToNum(addrType);

    QByteArray params;
    params.reserve(20);
    params.append(encodeInt32(startAddr));
    params.append(encodeInt32(endAddr));
    params.append(encodeInt32(typeNum));
    // dataType = 0x01 (word)
    params.append(encodeInt32(0x01));
    // dataLen = 2
    params.append(encodeInt32(0x00000002));

    QByteArray block = buildBlock(0x0002, 0x8002, params);
    block.resize(28);
    block.append(encodeInt16(value));

    return block;
}

QByteArray FanucFrameBuilder::buildWritePMCBlockLong(char addrType, int startAddr, int endAddr,
                                                       int pmcTypeNum, qint32 value)
{
    quint8 typeNum = pmcAddrTypeToNum(addrType);

    QByteArray params;
    params.reserve(20);
    params.append(encodeInt32(startAddr));
    params.append(encodeInt32(endAddr));
    params.append(encodeInt32(typeNum));
    // dataType = 0x02 (long)
    params.append(encodeInt32(0x02));
    // dataLen = 4
    params.append(encodeInt32(0x00000004));

    QByteArray block = buildBlock(0x0002, 0x8002, params);
    block.resize(28);
    block.append(encodeInt32(value));

    return block;
}

QByteArray FanucFrameBuilder::buildWritePMCBlockFloat(char addrType, int startAddr, int endAddr,
                                                        int pmcTypeNum, float value)
{
    quint8 typeNum = pmcAddrTypeToNum(addrType);

    QByteArray params;
    params.reserve(20);
    params.append(encodeInt32(startAddr));
    params.append(encodeInt32(endAddr));
    params.append(encodeInt32(typeNum));
    // dataType = 0x03 (float)
    params.append(encodeInt32(0x03));
    // dataLen = 4
    params.append(encodeInt32(0x00000004));

    QByteArray block = buildBlock(0x0002, 0x8002, params);
    block.resize(28);
    // Encode float as 4 bytes big-endian
    QByteArray floatBytes;
    floatBytes.resize(4);
    memcpy(floatBytes.data(), &value, 4);
    // Reverse for big-endian (assuming little-endian host)
    std::reverse(floatBytes.begin(), floatBytes.end());
    block.append(floatBytes);

    return block;
}

QByteArray FanucFrameBuilder::buildReadMacroBlock(int addr)
{
    QByteArray params;
    params.reserve(20);
    // startAddr (4B)
    params.append(encodeInt32(addr));
    // endAddr (4B)
    params.append(encodeInt32(addr));
    // zeros (12B)
    params.append(12, '\0');

    // Read macro: ncFlag=0x0001 (NC), blockFuncCode=0x0015
    return buildBlock(0x0001, 0x0015, params);
}

QByteArray FanucFrameBuilder::buildWriteMacroBlock(int addr, int numerator, quint8 precision)
{
    QByteArray params;
    params.reserve(20);
    // startAddr (4B)
    params.append(encodeInt32(addr));
    // endAddr (4B)
    params.append(encodeInt32(addr));
    // dataLen (4B) = 8
    params.append(encodeInt32(0x00000008));
    // zeros (4B) -- remaining params20 space
    params.append(4, '\0');
    // At this point params is 16 bytes; pad to 20
    params = params.leftJustified(20, '\0', true);

    // Build the 28B block header
    QByteArray block = buildBlock(0x0001, 0x0016, params);
    block.resize(28);

    // Append 8 extra bytes: numerator(4B) + denomBase(2B)=0x000A + precision(2B)
    block.append(encodeInt32(numerator));
    block.append(encodeInt16(0x000A));  // denomBase = 10
    block.append(encodeInt16(static_cast<qint16>(precision)));

    return block;
}

QByteArray FanucFrameBuilder::buildReadParamBlock(int paramNo)
{
    QByteArray params;
    params.reserve(20);
    // startAddr (4B)
    params.append(encodeInt32(paramNo));
    // endAddr (4B)
    params.append(encodeInt32(paramNo));
    // zeros (12B)
    params.append(12, '\0');

    // Read param: ncFlag=0x0001 (NC), blockFuncCode=0x008D
    return buildBlock(0x0001, 0x008D, params);
}

QByteArray FanucFrameBuilder::buildPositionBlock(quint8 posType)
{
    QByteArray params;
    params.reserve(20);
    // posType (4B, only last byte used)
    params.append(encodeInt32(posType));
    // 0xFFFFFFFF (4B, all axes)
    params.append(encodeInt32(-1));
    // zeros (12B)
    params.append(12, '\0');

    // Position: ncFlag=0x0001 (NC), blockFuncCode=0x0026
    return buildBlock(0x0001, 0x0026, params);
}

// ---------------------------------------------------------------------------
// Frame parsing
// ---------------------------------------------------------------------------

QByteArray FanucFrameBuilder::extractFrame(QByteArray& buffer)
{
    if (buffer.isEmpty())
        return QByteArray();

    // Check first byte is HEADER_BYTE
    if (static_cast<quint8>(buffer[0]) != HEADER_BYTE) {
        buffer.clear();
        return QByteArray();
    }

    // Need at least header(4) + func(4) + dataLen(2) = 10 bytes
    if (buffer.size() < 10)
        return QByteArray();

    // Read data length at offset 8-9 (big-endian)
    quint16 dataLen = static_cast<quint16>(
        (static_cast<quint8>(buffer[8]) << 8) | static_cast<quint8>(buffer[9]));

    int totalFrameSize = 10 + dataLen;

    if (buffer.size() < totalFrameSize)
        return QByteArray();

    QByteArray frame = buffer.left(totalFrameSize);
    buffer.remove(0, totalFrameSize);
    return frame;
}

bool FanucFrameBuilder::validateHeader(const QByteArray& frame)
{
    if (frame.size() < 10)
        return false;

    for (int i = 0; i < 4; ++i) {
        if (static_cast<quint8>(frame[i]) != HEADER_BYTE)
            return false;
    }

    return true;
}

quint16 FanucFrameBuilder::getDataLength(const QByteArray& frame)
{
    if (frame.size() < 10)
        return 0;

    return static_cast<quint16>(
        (static_cast<quint8>(frame[8]) << 8) | static_cast<quint8>(frame[9]));
}

// ---------------------------------------------------------------------------
// Data encoding (big-endian)
// ---------------------------------------------------------------------------

QByteArray FanucFrameBuilder::encodeInt32(qint32 value)
{
    QByteArray ba;
    ba.resize(4);
    ba[0] = static_cast<char>((value >> 24) & 0xFF);
    ba[1] = static_cast<char>((value >> 16) & 0xFF);
    ba[2] = static_cast<char>((value >> 8) & 0xFF);
    ba[3] = static_cast<char>(value & 0xFF);
    return ba;
}

qint32 FanucFrameBuilder::decodeInt32(const QByteArray& data, int offset)
{
    if (offset + 4 > data.size())
        return 0;

    return (static_cast<qint32>(static_cast<quint8>(data[offset])) << 24) |
           (static_cast<qint32>(static_cast<quint8>(data[offset + 1])) << 16) |
           (static_cast<qint32>(static_cast<quint8>(data[offset + 2])) << 8) |
           static_cast<qint32>(static_cast<quint8>(data[offset + 3]));
}

QByteArray FanucFrameBuilder::encodeInt16(qint16 value)
{
    QByteArray ba;
    ba.resize(2);
    ba[0] = static_cast<char>((value >> 8) & 0xFF);
    ba[1] = static_cast<char>(value & 0xFF);
    return ba;
}

qint16 FanucFrameBuilder::decodeInt16(const QByteArray& data, int offset)
{
    if (offset + 2 > data.size())
        return 0;

    return static_cast<qint16>(
        (static_cast<qint16>(static_cast<quint8>(data[offset])) << 8) |
         static_cast<quint8>(data[offset + 1]));
}

double FanucFrameBuilder::calcValue(const QByteArray& data, int offset)
{
    if (offset + 8 > data.size())
        return 0.0;

    qint32 numerator = decodeInt32(data, offset);
    qint16 denomBase = decodeInt16(data, offset + 4);
    qint16 denomExp  = decodeInt16(data, offset + 6);

    return static_cast<double>(numerator) / std::pow(static_cast<double>(denomBase),
                                                      static_cast<double>(denomExp));
}

// ---------------------------------------------------------------------------
// PMC helpers
// ---------------------------------------------------------------------------

quint8 FanucFrameBuilder::pmcAddrTypeToNum(char addrType)
{
    switch (addrType) {
    case 'G': return 0;
    case 'F': return 1;
    case 'Y': return 2;
    case 'X': return 3;
    case 'A': return 4;
    case 'R': return 5;
    case 'T': return 6;
    case 'K': return 7;
    case 'C': return 8;
    case 'D': return 9;
    default:  return 0;
    }
}

char FanucFrameBuilder::pmcNumToAddrType(quint8 num)
{
    static const char types[] = "GFYXARTKCD";
    if (num < 10)
        return types[num];
    return '?';
}
