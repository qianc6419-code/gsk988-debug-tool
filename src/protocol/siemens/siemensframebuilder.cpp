#include "siemensframebuilder.h"
#include <cstring>

// ========== S7 Frame Builder Helpers ==========

// Build a standard S7 Read Var request (single item, S7ANY addressing)
// Layout:
//   TPKT (4): 03 00 XX XX
//   COTP  (3): 02 F0 80
//   S7   (12): 32 01 00 00 00 01 00 0E 00 00 00 00
//   Param(14): 04 01 12 0A 10 TT NN NN DD DD AA XX XX XX
// Total = 33 bytes

static QByteArray buildReadVar(quint8 transportSize, quint16 numElements,
                                quint16 dbNumber, quint8 area,
                                quint8 addrHi, quint8 addrMid, quint8 addrLo)
{
    QByteArray frame(33, '\0');
    // TPKT
    frame[0] = 0x03;
    frame[1] = 0x00;
    frame[2] = 0x00;
    frame[3] = 0x21; // 33 bytes
    // COTP Data
    frame[4] = 0x02;
    frame[5] = (char)0xF0;
    frame[6] = (char)0x80;
    // S7 Header
    frame[7] = 0x32;  // Protocol ID
    frame[8] = 0x01;  // ROSCTR = Job
    frame[9] = 0x00; frame[10] = 0x00;  // Reserved
    frame[11] = 0x00; frame[12] = 0x01; // PDU Ref
    frame[13] = 0x00; frame[14] = 0x0E; // Param length = 14
    frame[15] = 0x00; frame[16] = 0x00; // Data length = 0
    frame[17] = 0x00; frame[18] = 0x00; // Error class/code
    // Parameter: Read Var, 1 item
    frame[19] = 0x04;  // Function: Read Var
    frame[20] = 0x01;  // Item count
    // S7ANY item
    frame[21] = 0x12;  // Variable spec
    frame[22] = 0x0A;  // Address spec length = 10
    frame[23] = 0x10;  // Syntax ID = S7ANY
    frame[24] = (char)transportSize;
    frame[25] = (char)((numElements >> 8) & 0xFF);
    frame[26] = (char)(numElements & 0xFF);
    frame[27] = (char)((dbNumber >> 8) & 0xFF);
    frame[28] = (char)(dbNumber & 0xFF);
    frame[29] = (char)area;
    frame[30] = (char)addrHi;
    frame[31] = (char)addrMid;
    frame[32] = (char)addrLo;
    return frame;
}

// Build a standard S7 Read Var request with 2 items
// Layout:
//   TPKT (4): 03 00 XX XX
//   COTP  (3): 02 F0 80
//   S7   (12): 32 01 00 00 00 01 00 1C 00 00 00 00
//   Param(30): 04 02  12 0A 10 TT1 N1 N1 D1 D1 A1 X1 X1 X1
//                      12 0A 10 TT2 N2 N2 D2 D2 A2 X2 X2 X2
// Total = 49 bytes

static QByteArray buildReadVar2Items(
    quint8 ts1, quint16 n1, quint16 db1, quint8 area1, quint8 a1hi, quint8 a1mid, quint8 a1lo,
    quint8 ts2, quint16 n2, quint16 db2, quint8 area2, quint8 a2hi, quint8 a2mid, quint8 a2lo)
{
    QByteArray frame(49, '\0');
    // TPKT
    frame[0] = 0x03;
    frame[1] = 0x00;
    frame[2] = 0x00;
    frame[3] = 0x31; // 49 bytes
    // COTP Data
    frame[4] = 0x02;
    frame[5] = (char)0xF0;
    frame[6] = (char)0x80;
    // S7 Header
    frame[7] = 0x32;
    frame[8] = 0x01;  // Job
    frame[9] = 0x00; frame[10] = 0x00;
    frame[11] = 0x00; frame[12] = 0x01; // PDU Ref
    frame[13] = 0x00; frame[14] = 0x1C; // Param length = 28
    frame[15] = 0x00; frame[16] = 0x00; // Data length = 0
    frame[17] = 0x00; frame[18] = 0x00;
    // Parameter
    frame[19] = 0x04;  // Read Var
    frame[20] = 0x02;  // 2 items
    // Item 1
    frame[21] = 0x12; frame[22] = 0x0A; frame[23] = 0x10;
    frame[24] = (char)ts1;
    frame[25] = (char)((n1 >> 8) & 0xFF); frame[26] = (char)(n1 & 0xFF);
    frame[27] = (char)((db1 >> 8) & 0xFF); frame[28] = (char)(db1 & 0xFF);
    frame[29] = (char)area1;
    frame[30] = (char)a1hi; frame[31] = (char)a1mid; frame[32] = (char)a1lo;
    // Item 2
    frame[33] = 0x12; frame[34] = 0x0A; frame[35] = 0x10;
    frame[36] = (char)ts2;
    frame[37] = (char)((n2 >> 8) & 0xFF); frame[38] = (char)(n2 & 0xFF);
    frame[39] = (char)((db2 >> 8) & 0xFF); frame[40] = (char)(db2 & 0xFF);
    frame[41] = (char)area2;
    frame[42] = (char)a2hi; frame[43] = (char)a2mid; frame[44] = (char)a2lo;
    return frame;
}

// ========== 握手帧 ==========

// HANDSHAKE_1: COTP Connect Request (22 bytes)
const QByteArray SiemensFrameBuilder::HANDSHAKE_1 =
    QByteArray::fromHex("0300001611e0000000480"
                        "0c1020400c2020d04c0010a");

// HANDSHAKE_2: S7 Communication Setup (25 bytes)
const QByteArray SiemensFrameBuilder::HANDSHAKE_2 =
    QByteArray::fromHex("0300001902f08032010000"
                        "00010008000"
                        "0f0000064006403c0");

// HANDSHAKE_3: S7 Setup (33 bytes)
// 使用 buildReadVar 构造正确的 Read Var 请求
const QByteArray SiemensFrameBuilder::HANDSHAKE_3 =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x04, 0x00);

// ===== 系统信息 =====

// CNC标识 (area=0x82, transport=0x02 byte, numElem varies)
const QByteArray SiemensFrameBuilder::CMD_CNC_ID =
    buildReadVar(0x02, 20, 0x0001, 0x82, 0x00, 0x46, 0x6E);

const QByteArray SiemensFrameBuilder::CMD_CNC_TYPE =
    buildReadVar(0x02, 20, 0x0004, 0x82, 0x00, 0x46, 0x78);

const QByteArray SiemensFrameBuilder::CMD_VER_INFO =
    buildReadVar(0x02, 20, 0x0001, 0x82, 0x00, 0x46, 0x78);

const QByteArray SiemensFrameBuilder::CMD_MANUF_DATE =
    buildReadVar(0x02, 20, 0x0003, 0x82, 0x00, 0x46, 0x78);

// ===== 运行状态 =====
// MODE uses 2 items to read mode + refpoint status

const QByteArray SiemensFrameBuilder::CMD_MODE =
    buildReadVar2Items(
        0x04, 1, 0x0001, 0x82, 0x00, 0x21, 0x00,  // item 1: mode
        0x04, 1, 0x0001, 0x82, 0x00, 0x41, 0x0C     // item 2: ref
    );

const QByteArray SiemensFrameBuilder::CMD_STATUS =
    buildReadVar2Items(
        0x04, 1, 0x0001, 0x82, 0x00, 0x41, 0x0B,  // item 1
        0x04, 1, 0x0001, 0x82, 0x00, 0x41, 0x0D     // item 2
    );

// ===== 主轴 =====

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_SET_SPEED =
    buildReadVar(0x02, 4, 0x0001, 0x82, 0x00, 0x00, 0x03);

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_ACT_SPEED =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x00);

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_RATE =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x04);

// ===== 进给 =====

const QByteArray SiemensFrameBuilder::CMD_FEED_SET_SPEED =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x02);

const QByteArray SiemensFrameBuilder::CMD_FEED_ACT_SPEED =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x01);

const QByteArray SiemensFrameBuilder::CMD_FEED_RATE =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x03);

// ===== 计数与时间 =====

const QByteArray SiemensFrameBuilder::CMD_PRODUCTS =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x79);

const QByteArray SiemensFrameBuilder::CMD_SET_PRODUCTS =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x77);

const QByteArray SiemensFrameBuilder::CMD_CYCLE_TIME =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x29);

const QByteArray SiemensFrameBuilder::CMD_REMAIN_TIME =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x2A);

// ===== 程序与刀具 =====

const QByteArray SiemensFrameBuilder::CMD_PROG_NAME =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x0C);

const QByteArray SiemensFrameBuilder::CMD_PROG_CONTENT =
    buildReadVar(0x02, 40, 0x0001, 0x82, 0x00, 0x41, 0x1F);

const QByteArray SiemensFrameBuilder::CMD_TOOL_NUM =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x17);

const QByteArray SiemensFrameBuilder::CMD_TOOL_D =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x23);

const QByteArray SiemensFrameBuilder::CMD_TOOL_H =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0xB6);

const QByteArray SiemensFrameBuilder::CMD_TOOL_X_LEN =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x71);

const QByteArray SiemensFrameBuilder::CMD_TOOL_Z_LEN =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x81, 0x11);

const QByteArray SiemensFrameBuilder::CMD_TOOL_RADIU =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x00);

const QByteArray SiemensFrameBuilder::CMD_TOOL_EDG =
    buildReadVar(0x02, 2, 0x0001, 0x82, 0x00, 0x41, 0x02);

// ===== 坐标 =====

const QByteArray SiemensFrameBuilder::CMD_MACH_POS =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x02);

const QByteArray SiemensFrameBuilder::CMD_WORK_POS =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x19);

const QByteArray SiemensFrameBuilder::CMD_REMAIN_POS =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x41, 0x03);

const QByteArray SiemensFrameBuilder::CMD_AXIS_NAME =
    buildReadVar(0x02, 20, 0x0001, 0x82, 0x00, 0x4E, 0x70);

// ===== 驱动诊断 =====

const QByteArray SiemensFrameBuilder::CMD_DRIVE_VOLTAGE =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x01, 0x1A);

const QByteArray SiemensFrameBuilder::CMD_DRIVE_CURRENT =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x01, 0x1E);

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD1 =
    buildReadVar(0x02, 1, 0x0001, 0xA2, 0x00, 0x01, 0x20);

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD2 =
    buildReadVar(0x02, 1, 0x0001, 0xA2, 0x00, 0x01, 0x1D);

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD3 =
    buildReadVar(0x02, 1, 0x0001, 0xA2, 0x00, 0x01, 0x1E);

const QByteArray SiemensFrameBuilder::CMD_DRIVE_LOAD4 =
    buildReadVar(0x02, 1, 0x0001, 0xA2, 0x00, 0x01, 0x1F);

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_LOAD1 =
    buildReadVar(0x02, 1, 0x0001, 0xA1, 0x00, 0x01, 0x21);

const QByteArray SiemensFrameBuilder::CMD_SPINDLE_LOAD2 =
    buildReadVar(0x02, 1, 0x0001, 0xA2, 0x00, 0x01, 0x21);

const QByteArray SiemensFrameBuilder::CMD_DRIVE_TEMPER =
    buildReadVar(0x02, 1, 0x0001, 0xA1, 0x00, 0x01, 0x23);

// ===== 告警 =====

const QByteArray SiemensFrameBuilder::CMD_ALARM_NUM =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x01, 0x07);

// CMD_ALARM uses 2 items
const QByteArray SiemensFrameBuilder::CMD_ALARM =
    buildReadVar2Items(
        0x04, 1, 0x0001, 0x82, 0x00, 0x01, 0x01,  // item 1: alarm index
        0x04, 1, 0x0001, 0x82, 0x00, 0x01, 0x04     // item 2
    );

// ===== 变量读写 =====

const QByteArray SiemensFrameBuilder::CMD_READ_R =
    buildReadVar(0x02, 1, 0x0001, 0x82, 0x00, 0x2C, 0x15);

// CMD_WRITE_R: S7 Write Var (41 bytes)
// 03 00 00 29  02 F0 80  32 01 00 00 00 01 00 12 00 0C 00 00
// 05 01 12 08 82 41 00 01 00 02 15 01 00 09 00 08 <8 bytes value>
const QByteArray SiemensFrameBuilder::CMD_WRITE_R =
    QByteArray("\x03\x00\x00\x29" // TPKT: 41
               "\x02\xF0\x80"     // COTP
               "\x32\x01"         // S7 + Job
               "\x00\x00"         // Reserved
               "\x00\x01"         // PDU Ref
               "\x00\x12"         // Param length = 18
               "\x00\x0C"         // Data length = 12
               "\x00\x00"         // Error
               "\x05\x01"         // Write Var, 1 item
               "\x12\x08\x82\x41" // Item: var spec, addr len=8
               "\x00\x01"         // Num elements
               "\x00\x02"         // DB
               "\x15\x01"         // Area + addr
               "\x00\x09"         // Transport size + padding
               "\x00\x08"         // Data length in data section
               "\x00\x00\x00\x00" // Value bytes (placeholder)
               "\x00\x00\x00\x00",// Value bytes (placeholder)
               41);

const QByteArray SiemensFrameBuilder::CMD_R_DRIVER =
    buildReadVar(0x02, 1, 0x0001, 0xA1, 0x00, 0x01, 0x25);

// ===== 工件坐标系 =====
// 3-item read request (49 bytes)

const QByteArray SiemensFrameBuilder::CMD_T_WORK_SYSTEM =
    buildReadVar2Items(
        0x04, 1, 0x0001, 0x82, 0x00, 0x01, 0x20,  // item 1
        0x04, 1, 0x0001, 0x82, 0x00, 0x01, 0x05     // item 2
    );

const QByteArray SiemensFrameBuilder::CMD_M_WORK_SYSTEM =
    buildReadVar2Items(
        0x04, 1, 0x0001, 0x82, 0x00, 0x01, 0x20,  // item 1
        0x04, 1, 0x0001, 0x82, 0x00, 0x01, 0x06     // item 2
    );

// ===== PLC (独立连接, 2次握手) =====

const QByteArray SiemensFrameBuilder::PLC_HANDSHAKE_1 =
    QByteArray::fromHex("0300001611e0000000010"
                        "0c0010ac1020102c2020102");

// PLC_HANDSHAKE_2: S7 Comm Setup (25 bytes)
const QByteArray SiemensFrameBuilder::PLC_HANDSHAKE_2 =
    QByteArray("\x03\x00\x00\x19" // TPKT: 25
               "\x02\xF0\x80"     // COTP
               "\x32\x01"         // S7 + Job
               "\x00\x00"         // Reserved
               "\x00\x00"         // PDU Ref = 0
               "\x00\x04"         // Param length = 4
               "\x00\x08"         // Data length = 8
               "\x00\xF0"         // Error
               "\x00\x00"         // Padding
               "\x01\x00"         // AMR
               "\x01\x01"         // UDT
               "\xE0",            // ...
               25);

const QByteArray SiemensFrameBuilder::CMD_PLC_READBYTE =
    QByteArray("\x03\x00\x00\x1F" // TPKT: 31
               "\x02\xF0\x80"     // COTP
               "\x32\x01"         // S7 + Job
               "\x00\x00"         // Reserved
               "\x00\x01"         // PDU Ref
               "\x00\x0E"         // Param length = 14
               "\x00\x00"         // Data length = 0
               "\x00\x00"         // Error
               "\x04\x01"         // Read Var, 1 item
               "\x12\x0A\x10"     // Var spec, addr len=10, S7ANY
               "\x02\x00\x01"     // Transport=byte, numElem=1
               "\x06\x40"         // DB=1600
               "\x84"             // Area=DB
               "\x00\x00\x00",    // Address=0 (byteOffset*8 set at runtime)
               31);


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
    // In buildReadVar format, the address field is at bytes 30-32
    // axisFlag goes into byte 32 (addrLo)
    if (cmd.size() > 32) cmd[32] = static_cast<char>(axisFlag);
    return cmd;
}

QByteArray SiemensFrameBuilder::buildAlarmCmd(int alarmIndex)
{
    QByteArray cmd(CMD_ALARM);
    // In buildReadVar2Items format, item 1's addrLo is at byte 32
    if (cmd.size() > 32) cmd[32] = static_cast<char>(alarmIndex);
    return cmd;
}

QByteArray SiemensFrameBuilder::buildReadRCmd(int rAddr)
{
    QByteArray cmd(CMD_READ_R);
    // In buildReadVar format, the address field is at bytes 30-32
    if (cmd.size() > 31) {
        cmd[30] = static_cast<char>((rAddr >> 8) & 0xFF);
        cmd[31] = static_cast<char>(rAddr & 0xFF);
    }
    return cmd;
}

QByteArray SiemensFrameBuilder::buildWriteRCmd(int rAddr, double value)
{
    QByteArray cmd(CMD_WRITE_R);
    // Write frame layout: address at bytes 28-29, value at bytes 33-40
    if (cmd.size() > 29) {
        cmd[28] = static_cast<char>((rAddr >> 8) & 0xFF);
        cmd[29] = static_cast<char>(rAddr & 0xFF);
    }
    // Replace the double value (last 8 bytes)
    QByteArray valBytes = encodeDouble(value);
    cmd.replace(cmd.size() - 8, 8, valBytes);
    return cmd;
}

QByteArray SiemensFrameBuilder::buildRDriverCmd(quint8 axisFlag, quint8 addrFlag, quint8 indexFlag)
{
    QByteArray cmd(CMD_R_DRIVER);
    // In buildReadVar format:
    // byte 29 = area, byte 30 = addrHi, byte 31 = addrMid, byte 32 = addrLo
    if (cmd.size() > 32) {
        cmd[30] = static_cast<char>(axisFlag);
        cmd[31] = static_cast<char>(addrFlag);
        cmd[32] = static_cast<char>(indexFlag);
    }
    return cmd;
}

QByteArray SiemensFrameBuilder::buildPLCReadCmd(int byteOffset)
{
    QByteArray cmd(CMD_PLC_READBYTE);
    // The address field (last 3 bytes): set to byteOffset * 8 (bit address)
    if (cmd.size() >= 3) {
        int bitAddr = byteOffset * 8;
        cmd[cmd.size() - 3] = static_cast<char>((bitAddr >> 16) & 0xFF);
        cmd[cmd.size() - 2] = static_cast<char>((bitAddr >> 8) & 0xFF);
        cmd[cmd.size() - 1] = static_cast<char>(bitAddr & 0xFF);
    }
    return cmd;
}

// ===== 响应校验 =====
bool SiemensFrameBuilder::isResponseOK(const QByteArray& frame)
{
    // S7 Read Var Ack Data: return code at byte 21
    // 0xFF = success, anything else = error
    if (frame.size() < 22) return false;
    return static_cast<quint8>(frame[21]) == 0xFF;
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
    // S7 响应格式: TPKT头(4) + COTP头(3) + S7头(10) + 参数段 + 数据段
    // 数据段通常从 offset 25 开始
    const int dataOffset = 25;

    if (frame.size() < dataOffset + 8) {
        return 0.0;
    }

    // 从 offset 25 读取 8 字节 double
    double value;
    std::memcpy(&value, frame.constData() + dataOffset, 8);
    return value;
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
