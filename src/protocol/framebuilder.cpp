#include "framebuilder.h"
#include "../protocol/crc16.h"
#include <QDataStream>

QByteArray FrameBuilder::buildFrame(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data)
{
    quint16 length = 6 + data.size();

    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Header
    stream << FRAME_HEADER_HIGH;
    stream << FRAME_HEADER_LOW;

    // Length
    stream << length;

    // CMD, SUB, Type
    stream << cmd;
    stream << sub;
    stream << type;

    // Data
    frame.append(data);

    // Calculate XOR over CMD+SUB+Type+Data
    QByteArray xorData;
    xorData.append(cmd);
    xorData.append(sub);
    xorData.append(type);
    xorData.append(data);
    quint8 xorByte = calculateXor(xorData);
    frame.append(xorByte);

    // Calculate CRC16 over entire frame (before CRC is appended)
    quint16 crc = Crc16::calculate(frame);

    // Append CRC16 (2 bytes, LittleEndian)
    stream << crc;

    return frame;
}

bool FrameBuilder::parseFrame(const QByteArray &frame, quint8 &outCmd, quint8 &outSub,
                              quint8 &outType, QByteArray &outData, QString &error)
{
    // Minimum frame size: header(2) + length(2) + CMD(1) + SUB(1) + Type(1) + XOR(1) + CRC16(2) = 10
    if (frame.size() < 10) {
        error = "Frame too short";
        return false;
    }

    // Validate header
    if (!isValidFrameHeader(frame)) {
        error = "Invalid frame header";
        return false;
    }

    // Read length
    quint16 length = getFrameLength(frame);
    if (frame.size() < length) {
        error = "Frame length mismatch";
        return false;
    }

    // Extract CMD, SUB, Type
    outCmd = static_cast<quint8>(frame[4]);
    outSub = static_cast<quint8>(frame[5]);
    outType = static_cast<quint8>(frame[6]);

    // Extract data (from offset 7 to length-3, since XOR is at length-3 and CRC16 at length-2 and length-1)
    int dataSize = length - 8; // 6 (header+length+cmd+sub+type) + 1 (xor) + 1 (crc16 part 1) = 8, but actually length includes everything
    // Actually: header(2) + length(2) + cmd(1) + sub(1) + type(1) + data(n) + xor(1) + crc16(2) = length
    // So dataSize = length - 2 - 2 - 1 - 1 - 1 - 1 - 2 = length - 10
    dataSize = length - 10;
    outData = frame.mid(7, dataSize);

    // Verify XOR
    quint8 receivedXor = static_cast<quint8>(frame[length - 3]);
    QByteArray xorData;
    xorData.append(outCmd);
    xorData.append(outSub);
    xorData.append(outType);
    xorData.append(outData);
    quint8 calculatedXor = calculateXor(xorData);
    if (receivedXor != calculatedXor) {
        error = "XOR checksum mismatch";
        return false;
    }

    // Verify CRC16
    QByteArray frameForCrc = frame.left(length - 2);
    quint16 receivedCrc = static_cast<quint16>(frame[length - 2]) | (static_cast<quint16>(static_cast<quint8>(frame[length - 1])) << 8);
    quint16 calculatedCrc = Crc16::calculate(frameForCrc);
    if (receivedCrc != calculatedCrc) {
        error = "CRC16 checksum mismatch";
        return false;
    }

    return true;
}

quint8 FrameBuilder::calculateXor(const QByteArray &data)
{
    quint8 xorResult = 0;
    for (int i = 0; i < data.size(); ++i) {
        xorResult ^= static_cast<quint8>(data[i]);
    }
    return xorResult;
}

bool FrameBuilder::isValidFrameHeader(const QByteArray &frame)
{
    if (frame.size() < 2) {
        return false;
    }
    return static_cast<quint8>(frame[0]) == FRAME_HEADER_HIGH &&
           static_cast<quint8>(frame[1]) == FRAME_HEADER_LOW;
}

quint16 FrameBuilder::getFrameLength(const QByteArray &frame)
{
    if (frame.size() < 4) {
        return 0;
    }
    QDataStream stream(frame);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint16 length;
    stream >> length;
    return length;
}