#ifndef CRC16_H
#define CRC16_H

#include <QByteArray>

class Crc16
{
public:
    static quint16 calculate(const QByteArray &data);
    static quint16 calculate(const quint8 *data, int length);
    static bool verify(const QByteArray &data, quint16 expectedCrc);
};

#endif // CRC16_H
