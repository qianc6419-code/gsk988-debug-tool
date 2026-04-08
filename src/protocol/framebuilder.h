#ifndef FRAMEBUILDER_H
#define FRAMEBUILDER_H

#include <QByteArray>
class FrameBuilder
{
public:
    static QByteArray buildFrame(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data);
    static bool parseFrame(const QByteArray &frame, quint8 &outCmd, quint8 &outSub,
                         quint8 &outType, QByteArray &outData, QString &error);
    static quint8 calculateXor(const QByteArray &data);
    static bool isValidFrameHeader(const QByteArray &frame);
    static quint16 getFrameLength(const QByteArray &frame);
private:
    static const quint8 FRAME_HEADER_HIGH = 0x5A;
    static const quint8 FRAME_HEADER_LOW = 0xA5;
};
#endif