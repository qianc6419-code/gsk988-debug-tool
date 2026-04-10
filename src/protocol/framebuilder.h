#ifndef FRAMEBUILDER_H
#define FRAMEBUILDER_H

#include <QByteArray>

class FrameBuilder
{
public:
    // Build a request frame with the given data field
    static QByteArray buildRequestFrame(const QByteArray& dataField);
    // Build a response frame with the given data field
    static QByteArray buildResponseFrame(const QByteArray& dataField);
    // Extract one complete frame from the receive buffer
    static QByteArray extractFrame(QByteArray& buffer);
    // Validate frame integrity (head, tail, length consistency)
    static bool validateFrame(const QByteArray& frame);

    static constexpr char FRAME_HEAD_0 = static_cast<char>(0x93);
    static constexpr char FRAME_HEAD_1 = static_cast<char>(0x00);
    static constexpr char FRAME_TAIL_0 = static_cast<char>(0x55);
    static constexpr char FRAME_TAIL_1 = static_cast<char>(0xAA);

    static const QByteArray REQUEST_ID;
    static const QByteArray RESPONSE_ID;
};

#endif // FRAMEBUILDER_H
