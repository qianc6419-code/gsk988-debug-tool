#ifndef SYNTECFRAMEBUILDER_H
#define SYNTECFRAMEBUILDER_H

#include <QByteArray>
#include <QString>

class SyntecFrameBuilder
{
public:
    // Build request packet for given command code with tid
    static QByteArray buildPacket(quint8 cmdCode, quint8 tid);

    // Response validation
    static bool checkFrame(const QByteArray& frame, quint8 expectedTid);
    static quint8 extractTid(const QByteArray& frame);

    // Data extraction from response (offset relative to frame start)
    static qint32 extractInt32(const QByteArray& frame, int offset);
    static qint64 extractInt64(const QByteArray& frame, int offset);
    static QString extractString(const QByteArray& frame, int offset);

    // State mapping
    static QString statusToString(int status);
    static QString modeToString(int mode);
    static QString emgToString(int emg);
    static QString formatTime(int seconds);

    // Empty response detection (12-byte keepalive)
    static bool isEmptyResponse(const QByteArray& data);
};

#endif // SYNTECFRAMEBUILDER_H
