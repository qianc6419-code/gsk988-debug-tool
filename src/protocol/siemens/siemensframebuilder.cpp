#include "siemensframebuilder.h"
#include <cstring>

// ===== 握手帧 =====
const QByteArray SiemensFrameBuilder::HANDSHAKE_1 =
    QByteArray::fromHex("0300001611e00000 004800c1020400c2 020d04c0010a");

const QByteArray SiemensFrameBuilder::HANDSHAKE_2 =
    QByteArray::fromHex("0300001902f08032 0100000001000800 00f0000064006403 c0");

const QByteArray SiemensFrameBuilder::HANDSHAKE_3 =
    QByteArray::fromHex("0300001d02f08032 01000000010000c0 0000040112088201 00140001"
                        "3b010300000702 f000");

// ===== 系统信息 =====
const QByteArray SiemensFrameBuilder::CMD_CNC_ID =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 466e0001"
                        "1a010300000702 f000");

const QByteArray SiemensFrameBuilder::CMD_CNC_TYPE =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 46780004"
                        "1a010300000702 f000");

const QByteArray SiemensFrameBuilder::CMD_VER_INFO =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 46780001"
                        "1a010300000702 f000");

const QByteArray SiemensFrameBuilder::CMD_MANUF_DATE =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 46780003"
                        "1a010300000702 f000");

// ===== 运行状态 =====
const QByteArray SiemensFrameBuilder::CMD_MODE =
    QByteArray::fromHex("0300002702f08032 01000000140000c1 6000000402120882 21000300"
                        "017f0112088241 000c00017f0103 0000000702f000");

const QByteArray SiemensFrameBuilder::CMD_STATUS =
    QByteArray::fromHex("0300002702f08032 01000000140000c1 6000000402120882 41000b00"
                        "017f0112088241 000d00017f0103 0000000702f000");

// ===== 主轴 =====
const QByteArray SiemensFrameBuilder::CMD_SPINDLE_SET_SPEED =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 000300"
                        "04720103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_ACT_SPEED =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000200"
                        "01720103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_RATE =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000400"
                        "01720103000 007020f000");

// ===== 进给 =====
const QByteArray SiemensFrameBuilder::CMD_FEED_SET_SPEED =
    QByteArray::fromHex("0300001d02f08032 01000000120000c0 0000040112088241 000200"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_FEED_ACT_SPEED =
    QByteArray::fromHex("0300001d02f08032 01000000120000c0 0000040112088241 000100"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_FEED_RATE =
    QByteArray::fromHex("0300001d02f08032 01000000130000c0 0000040112088241 000300"
                        "017f01030000 07020f000");

// ===== 计数与时间 =====
const QByteArray SiemensFrameBuilder::CMD_PRODUCTS =
    QByteArray::fromHex("0300001d02f08032 01000000110000c0 0000040112088241 007900"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_SET_PRODUCTS =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 007700"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_CYCLE_TIME =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 012900"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_REMAIN_TIME =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 012a00"
                        "017f01030000 07020f000");

// ===== 程序与刀具 =====
const QByteArray SiemensFrameBuilder::CMD_PROG_NAME =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000c00"
                        "017a01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_PROG_CONTENT =
    QByteArray::fromHex("0300001d02f08032 01000000500000c0 0000040112088241 001f00"
                        "017d01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_NUM =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 001700"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_D =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 002300"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_H =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 01b600"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_X_LEN =
    QByteArray::fromHex("0300001d02f08032 01000000210000c0 0000040112088241 007100"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_Z_LEN =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088281 001100"
                        "01142303000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_RADIU =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000000"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_TOOL_EDG =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000200"
                        "02140103000 007020f000");

// ===== 坐标 =====
const QByteArray SiemensFrameBuilder::CMD_MACH_POS =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000200"
                        "01740103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_WORK_POS =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 001900"
                        "01700103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_REMAIN_POS =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088241 000300"
                        "01740103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_AXIS_NAME =
    QByteArray::fromHex("0300001d02f08032 01000000300000c0 0000040112088241 4e7000"
                        "011a0a03000 007020f000");

// ===== 驱动诊断 =====
const QByteArray SiemensFrameBuilder::CMD_DRIVE_VOLTAGE =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 001a00"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_DRIVE_CURRENT =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 0000040112088201 001e00"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD1 =
    QByteArray::fromHex("0300001d02f08032 01000000150000c0 00000401120882a2 002000"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD2 =
    QByteArray::fromHex("0300001d02f08032 01000000160000c0 00000401120882a2 001d00"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD3 =
    QByteArray::fromHex("0300001d02f08032 01000000170000c0 00000401120882a2 001e00"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD4 =
    QByteArray::fromHex("0300001d02f08032 01000000180000c0 00000401120882a2 001f00"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_LOAD1 =
    QByteArray::fromHex("0300001d02f08032 01000000150000c0 00000401120882a1 002100"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_LOAD2 =
    QByteArray::fromHex("0300001d02f08032 01000000150000c0 00000401120882a2 002100"
                        "01820103000 007020f000");

const QByteArray SiemensFrameBuilder::CMD_DRIVE_TEMPER =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 00000401120882a1 002300"
                        "01820103000 007020f000");

// ===== 告警 =====
const QByteArray SiemensFrameBuilder::CMD_ALARM_NUM =
    QByteArray::fromHex("0300001d02f08032 010000000c0000c0 0000040112088201 000700"
                        "017f01030000 07020f000");

const QByteArray SiemensFrameBuilder::CMD_ALARM =
    QByteArray::fromHex("0300002702f08032 010000000d0000c1 6000000402120882 01000100"
                        "017701120882 010004000177 01");

// ===== 变量读写 =====
const QByteArray SiemensFrameBuilder::CMD_READ_R =
    QByteArray::fromHex("0300001d02f08032 01000000100000c0 0000040112088241 000101"
                        "012c15010300 0007020f000");

const QByteArray SiemensFrameBuilder::CMD_WRITE_R =
    QByteArray::fromHex("0300002902f08032 010000001200000c 000c050112088241 000100"
                        "021501000900 08000000000044f4 2041");

const QByteArray SiemensFrameBuilder::CMD_R_DRIVER =
    QByteArray::fromHex("0300001d02f08032 01000000140000c0 00000401120882a1 002500"
                        "01820103000 007020f000");

// ===== 工件坐标系 =====
const QByteArray SiemensFrameBuilder::CMD_T_WORK_SYSTEM =
    QByteArray::fromHex("0300003102f0803201000000020000200000040312088241000100012001"
                        "1208824100010005120112088241000100061201");

const QByteArray SiemensFrameBuilder::CMD_M_WORK_SYSTEM =
    QByteArray::fromHex("0300003102f0803201000000020000200000040312088241000100012001"
                        "1208824100010005120112088241000100061201");

// ===== PLC (独立连接, 2次握手) =====
const QByteArray SiemensFrameBuilder::PLC_HANDSHAKE_1 =
    QByteArray::fromHex("0300001611e000000001 00c0010ac1020102c2 020102");

const QByteArray SiemensFrameBuilder::PLC_HANDSHAKE_2 =
    QByteArray::fromHex("0300001902f08032010000000400000800 00f0000001000101e0");

const QByteArray SiemensFrameBuilder::CMD_PLC_READBYTE =
    QByteArray::fromHex("0300001f02f080320100000001000e00000401120a1002000106408400000000");


// ===== 帧提取 =====
QByteArray SiemensFrameBuilder::extractFrame(QByteArray& buffer)
{
    // TPKT: bytes 0=0x03, 1=0x00, 2-3=总长度(大端16位)
    if (buffer.size() < 4) return QByteArray();
    if (static_cast<quint8>(buffer[0]) != 0x03) {
        buffer.clear();
        return QByteArray();
    }
    quint16 totalLen = (static_cast<quint8>(buffer[2]) << 8) |
                        static_cast<quint8>(buffer[3]);
    if (buffer.size() < totalLen) return QByteArray();
    QByteArray frame = buffer.left(totalLen);
    buffer.remove(0, totalLen);
    return frame;
}

// ===== 参数化命令构建 =====
QByteArray SiemensFrameBuilder::buildPositionCmd(const QByteArray& base, quint8 axisFlag)
{
    QByteArray cmd(base);
    if (cmd.size() > 26) cmd[26] = static_cast<char>(axisFlag);
    return cmd;
}

QByteArray SiemensFrameBuilder::buildAlarmCmd(int alarmIndex)
{
    QByteArray cmd(CMD_ALARM);
    if (cmd.size() > 26) cmd[26] = static_cast<char>(alarmIndex);
    return cmd;
}

QByteArray SiemensFrameBuilder::buildReadRCmd(int rAddr)
{
    QByteArray cmd(CMD_READ_R);
    if (cmd.size() > 26) {
        // C# code: [25] = flag[1], [26] = flag[0]
        // rAddr is combined, split into high/low bytes
        cmd[25] = static_cast<char>((rAddr >> 8) & 0xFF);
        cmd[26] = static_cast<char>(rAddr & 0xFF);
    }
    return cmd;
}

QByteArray SiemensFrameBuilder::buildWriteRCmd(int rAddr, double value)
{
    QByteArray cmd(CMD_WRITE_R);
    if (cmd.size() > 26) {
        cmd[25] = static_cast<char>((rAddr >> 8) & 0xFF);
        cmd[26] = static_cast<char>(rAddr & 0xFF);
    }
    // Replace the double value (last 8 bytes)
    QByteArray valBytes = encodeDouble(value);
    cmd.replace(cmd.size() - 8, 8, valBytes);
    return cmd;
}

QByteArray SiemensFrameBuilder::buildRDriverCmd(quint8 axisFlag, quint8 addrFlag, quint8 indexFlag)
{
    QByteArray cmd(CMD_R_DRIVER);
    // C# code: [22] = axisFlag, [24] = addrFlag, [26] = indexFlag
    if (cmd.size() > 26) {
        cmd[22] = static_cast<char>(axisFlag);
        cmd[24] = static_cast<char>(addrFlag);
        cmd[26] = static_cast<char>(indexFlag);
    }
    return cmd;
}

QByteArray SiemensFrameBuilder::buildPLCReadCmd(int byteOffset)
{
    QByteArray cmd(CMD_PLC_READBYTE);
    if (cmd.size() > 0) {
        cmd[cmd.size() - 1] = static_cast<char>(byteOffset * 8);
    }
    return cmd;
}

// ===== 响应解析 =====
QString SiemensFrameBuilder::parseString(const QByteArray& frame)
{
    if (frame.size() <= 25) return QString();
    QByteArray strBytes = frame.mid(25);
    QString s = QString::fromUtf8(strBytes.constData(), strBytes.size());
    s.remove(QChar('\0'));
    return s.isEmpty() ? QStringLiteral("未加工") : s;
}

float SiemensFrameBuilder::parseFloat(const QByteArray& frame)
{
    if (frame.size() < 29) return 0.0f;
    // C# code: BitConverter.ToSingle(datas.Skip(25).Reverse().ToArray(), 0)
    QByteArray bytes = frame.mid(25, 4);
    std::reverse(bytes.begin(), bytes.end());
    float value;
    std::memcpy(&value, bytes.constData(), 4);
    return value;
}

double SiemensFrameBuilder::parseDouble(const QByteArray& frame)
{
    if (frame.size() < 33) {
        if (frame.size() >= 8) {
            double value;
            std::memcpy(&value, frame.constData(), 8);
            return value;
        }
        return 0.0;
    }
    // C# code: if (datas[3] == 33) → take 8 bytes from offset 25
    if (static_cast<quint8>(frame[3]) == 33) {
        double value;
        std::memcpy(&value, frame.constData() + 25, 8);
        return value;
    } else {
        double value;
        std::memcpy(&value, frame.constData(), 8);
        return value;
    }
}

qint32 SiemensFrameBuilder::parseInt32(const QByteArray& frame)
{
    if (frame.size() < 29) return 0;
    // C# code for alarm: ToInt32(datas.Skip(25).Take(4), 0) — little-endian
    qint32 value;
    std::memcpy(&value, frame.constData() + 25, 4);
    return value;
}

qint16 SiemensFrameBuilder::parseInt16(const QByteArray& frame)
{
    if (frame.size() < 27) return 0;
    // C# code: ToInt16(datas.Skip(25).Take(2), 0) — little-endian
    qint16 value;
    std::memcpy(&value, frame.constData() + 25, 2);
    return value;
}

QString SiemensFrameBuilder::parseMode(const QByteArray& frame)
{
    if (frame.size() < 32) return QStringLiteral("OTHER");
    if (static_cast<quint8>(frame[24]) != 0x02) return QStringLiteral("OTHER");

    quint8 b25 = static_cast<quint8>(frame[25]);
    quint8 b31 = static_cast<quint8>(frame[31]);

    if (b31 == 0x00) {
        if (b25 == 0x00) return QStringLiteral("JOG");
        if (b25 == 0x01) return QStringLiteral("MDA");
        if (b25 == 0x02) return QStringLiteral("AUTO");
        return QStringLiteral("OTHER");
    } else if (b31 == 0x03) {
        return QStringLiteral("REFPOINT");
    }
    return QStringLiteral("OTHER");
}

QString SiemensFrameBuilder::parseStatus(const QByteArray& frame)
{
    if (frame.size() < 32) return QStringLiteral("OTHER");
    if (static_cast<quint8>(frame[24]) != 0x02) return QStringLiteral("OTHER");

    quint8 b25 = static_cast<quint8>(frame[25]);
    quint8 b31 = static_cast<quint8>(frame[31]);

    if (b25 == 0x00 && b31 == 0x05) return QStringLiteral("RESET");
    if (b25 == 0x02 && b31 == 0x02) return QStringLiteral("STOP");
    if (b25 == 0x01 && b31 == 0x03) return QStringLiteral("START");
    if (b25 == 0x01 && b31 == 0x05) return QStringLiteral("SPINDLE_CW_CCW");
    return QStringLiteral("OTHER");
}

// ===== 数据类型编码 =====
QByteArray SiemensFrameBuilder::encodeDouble(double value)
{
    QByteArray bytes(8, '\0');
    std::memcpy(bytes.data(), &value, 8);
    return bytes;
}

QByteArray SiemensFrameBuilder::encodeFloat(float value)
{
    QByteArray bytes(4, '\0');
    std::memcpy(bytes.data(), &value, 4);
    return bytes;
}
