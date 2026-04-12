#ifndef GSK988FRAMEBUILDER_H
#define GSK988FRAMEBUILDER_H

#include <QByteArray>

class Gsk988FrameBuilder
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

    static constexpr quint8 FRAME_HEAD_0 = 0x93;
    static constexpr quint8 FRAME_HEAD_1 = 0x00;
    static constexpr quint8 FRAME_TAIL_0 = 0x55;
    static constexpr quint8 FRAME_TAIL_1 = 0xAA;

    static const QByteArray REQUEST_ID;
    static const QByteArray RESPONSE_ID;
};

#endif // GSK988FRAMEBUILDER_H
