#include "framebuilder.h"

const QByteArray FrameBuilder::REQUEST_ID  = QByteArray::fromHex("6fc81e641e171017");
const QByteArray FrameBuilder::RESPONSE_ID = QByteArray::fromHex("00641ec81e171001");

QByteArray FrameBuilder::buildRequestFrame(const QByteArray& dataField)
{
    // Length = REQUEST_ID(8) + dataField
    quint16 len = static_cast<quint16>(REQUEST_ID.size() + dataField.size());

    QByteArray frame;
    frame.reserve(2 + 2 + len + 2);

    // Head
    frame.append(FRAME_HEAD_0);
    frame.append(FRAME_HEAD_1);

    // Length (little-endian)
    frame.append(static_cast<char>(len & 0xFF));
    frame.append(static_cast<char>((len >> 8) & 0xFF));

    // Identifier + data field
    frame.append(REQUEST_ID);
    frame.append(dataField);

    // Tail
    frame.append(FRAME_TAIL_0);
    frame.append(FRAME_TAIL_1);

    return frame;
}

QByteArray FrameBuilder::buildResponseFrame(const QByteArray& dataField)
{
    quint16 len = static_cast<quint16>(RESPONSE_ID.size() + dataField.size());

    QByteArray frame;
    frame.reserve(2 + 2 + len + 2);

    frame.append(FRAME_HEAD_0);
    frame.append(FRAME_HEAD_1);

    frame.append(static_cast<char>(len & 0xFF));
    frame.append(static_cast<char>((len >> 8) & 0xFF));

    frame.append(RESPONSE_ID);
    frame.append(dataField);

    frame.append(FRAME_TAIL_0);
    frame.append(FRAME_TAIL_1);

    return frame;
}

QByteArray FrameBuilder::extractFrame(QByteArray& buffer)
{
    // Find frame head 0x93 0x00
    int headPos = -1;
    for (int i = 0; i <= buffer.size() - 2; ++i) {
        if (static_cast<quint8>(buffer[i]) == FRAME_HEAD_0 &&
            static_cast<quint8>(buffer[i + 1]) == FRAME_HEAD_1) {
            headPos = i;
            break;
        }
    }

    if (headPos < 0)
        return QByteArray();

    // Discard garbage before head
    if (headPos > 0)
        buffer.remove(0, headPos);

    // Need at least head(2) + length(2) = 4 bytes
    if (buffer.size() < 4)
        return QByteArray();

    // Read length (little-endian), bytes at index 2 and 3
    quint16 len = static_cast<quint8>(buffer[2]) |
                  (static_cast<quint8>(buffer[3]) << 8);

    // Complete frame = head(2) + length(2) + len bytes + tail(2) = 6 + len
    int totalFrameSize = 6 + len;

    if (buffer.size() < totalFrameSize)
        return QByteArray();

    // Verify tail
    if (static_cast<quint8>(buffer[totalFrameSize - 2]) != FRAME_TAIL_0 ||
        static_cast<quint8>(buffer[totalFrameSize - 1]) != FRAME_TAIL_1) {
        // Invalid tail — discard this head and search for next
        buffer.remove(0, 2);
        return extractFrame(buffer);
    }

    QByteArray frame = buffer.left(totalFrameSize);
    buffer.remove(0, totalFrameSize);
    return frame;
}

bool FrameBuilder::validateFrame(const QByteArray& frame)
{
    if (frame.size() < 6) // minimum: head(2) + length(2) + tail(2)
        return false;

    if (static_cast<quint8>(frame[0]) != FRAME_HEAD_0 ||
        static_cast<quint8>(frame[1]) != FRAME_HEAD_1)
        return false;

    quint16 len = static_cast<quint8>(frame[2]) |
                  (static_cast<quint8>(frame[3]) << 8);

    if (frame.size() != 6 + len)
        return false;

    if (static_cast<quint8>(frame[frame.size() - 2]) != FRAME_TAIL_0 ||
        static_cast<quint8>(frame[frame.size() - 1]) != FRAME_TAIL_1)
        return false;

    return true;
}
