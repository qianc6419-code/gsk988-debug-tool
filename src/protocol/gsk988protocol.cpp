#include "gsk988protocol.h"

Gsk988Protocol::Gsk988Protocol(QObject *parent)
    : QObject(parent)
{
}

Gsk988Protocol::~Gsk988Protocol()
{
}

QByteArray Gsk988Protocol::buildFrame(quint8 command, const QByteArray &payload)
{
    Q_UNUSED(command);
    Q_UNUSED(payload);
    return QByteArray();
}

bool Gsk988Protocol::parseFrame(const QByteArray &rawData)
{
    Q_UNUSED(rawData);
    return false;
}

bool Gsk988Protocol::validateCrc(const Gsk988Frame &frame)
{
    Q_UNUSED(frame);
    return false;
}
