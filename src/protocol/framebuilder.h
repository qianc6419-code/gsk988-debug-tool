#ifndef FRAMEBUILDER_H
#define FRAMEBUILDER_H

#include <QByteArray>

class FrameBuilder
{
public:
    FrameBuilder();

    void reset();
    void setHeader(quint8 header);
    void setCommand(quint8 command);
    void setPayload(const QByteArray &payload);
    QByteArray build();

private:
    quint8 m_header;
    quint8 m_command;
    QByteArray m_payload;
};

#endif // FRAMEBUILDER_H
