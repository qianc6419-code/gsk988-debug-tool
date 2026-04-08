#ifndef CRC16_H
#define CRC16_H

#include <QtGlobal>

class Crc16
{
public:
    static quint16 calculate(const quint8 *data, size_t length);
    static quint16 calculate(const QByteArray &data);
    static bool verify(const quint8 *data, size_t length, quint16 expectedCrc);
    static bool verify(const QByteArray &data, quint16 expectedCrc);

private:
    static const quint16 s_crcTable[256];
};

#endif // CRC16_H
