#include "framebuilder.h"

FrameBuilder::FrameBuilder()
{
}

void FrameBuilder::reset()
{
}

void FrameBuilder::setHeader(quint8 header)
{
    Q_UNUSED(header);
}

void FrameBuilder::setCommand(quint8 command)
{
    Q_UNUSED(command);
}

void FrameBuilder::setPayload(const QByteArray &payload)
{
    Q_UNUSED(payload);
}

QByteArray FrameBuilder::build()
{
    return QByteArray();
}
