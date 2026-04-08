#include "crc16.h"

quint16 Crc16::calculate(const quint8 *data, int length)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < length; ++i) {
        quint8 byte = data[i];
        crc ^= byte;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0x8005;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

quint16 Crc16::calculate(const QByteArray &data)
{
    return calculate(reinterpret_cast<const quint8*>(data.data()), data.size());
}

bool Crc16::verify(const QByteArray &data, quint16 expectedCrc)
{
    return calculate(data) == expectedCrc;
}
