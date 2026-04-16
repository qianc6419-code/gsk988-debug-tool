#include "syntecframebuilder.h"
#include <cstring>

// ===== Vendor ID configuration =====
// 默认 Vendor ID: 0xC8 (新代标准)
static quint8 s_vendorId = 0xC8;

// 设置 Vendor ID（用于不同型号的 Syntec 设备）
void SyntecFrameBuilder::setVendorId(quint8 vendorId)
{
    s_vendorId = vendorId;
}

// 获取当前 Vendor ID
quint8 SyntecFrameBuilder::getVendorId()
{
    return s_vendorId;
}

// ===== Packet templates (tid at byte[10] is placeholder, set dynamically) =====

struct SyntecCmdDef {
    quint8 cmdCode;
    const char* hex;
};

static const SyntecCmdDef s_cmdDefs[] = {
    {0x00, "1400000010000000c8000000c80000004f040000040000005000000001000000"}, // Handshake
    {0x01, "1400000010000000c8000100c80000008c040000040000000402000001000000"}, // ProgramName
    {0x02, "1400000010000000c8000200c800000007040000040000000800000005000000"}, // Mode
    {0x03, "1400000010000000c8000300c800000007040000040000000800000004000000"}, // Status
    {0x04, "1000000010000000c8000400c80000004a0400000000000008000000"},         // Alarm
    {0x05, "1800000010000000c8000500c80000000c04000008000000090000002400000001000000"}, // EMG
    {0x06, "1400000010000000c8000600c80000001a0400000400000008000000f4030000"}, // RunTime
    {0x07, "1400000010000000c8000700c80000001a0400000400000008000000f3030000"}, // CutTime
    {0x08, "1400000010000000c8000800c80000001a0400000400000008000000f2030000"}, // CycleTime
    {0x09, "1400000010000000c8000b00c80000001a0400000400000008000000e8030000"}, // Products
    {0x0A, "1400000010000000c8000c00c80000001a0400000400000008000000ea030000"}, // RequiredProducts
    {0x0B, "1400000010000000c8000d00c80000001a0400000400000008000000ec030000"}, // TotalProducts
    {0x0C, "1400000010000000c8000e00c800000007040000040000000800000013000000"}, // FeedOverride
    {0x0D, "1400000010000000c8000f00c800000007040000040000000800000015000000"}, // SpindleOverride
    {0x0E, "1400000010000000c8001000c80000001a040000040000000800000003030000"}, // SpindleSpeed
    {0x0F, "1400000010000000c8001100c80000001a0400000400000008000000bc020000"}, // FeedRateOri
    {0x10, "1400000010000000c8001200c80000000704000004000000080000000c000000"}, // DecPoint
    {0x20, "1800000010000000c8001f00c80000000504000008000000140000008d00000003000000"}, // RelativeAxes
    {0x21, "1800000010000000c8001c00c80000000504000008000000140000006500000003000000"}, // MachineAxes
};
static const int s_cmdDefCount = sizeof(s_cmdDefs) / sizeof(s_cmdDefs[0]);

// ===== Build =====

QByteArray SyntecFrameBuilder::buildPacket(quint8 cmdCode, quint8 tid)
{
    for (int i = 0; i < s_cmdDefCount; ++i) {
        if (s_cmdDefs[i].cmdCode == cmdCode) {
            QByteArray pkt = QByteArray::fromHex(s_cmdDefs[i].hex);

            // 设置 tid（位置 10）
            if (pkt.size() > 10)
                pkt[10] = static_cast<char>(tid);

            // 替换硬编码的 Vendor ID（位置 9 和 11）
            if (pkt.size() > 11) {
                pkt[9] = static_cast<char>(s_vendorId);
                pkt[11] = static_cast<char>(s_vendorId);
            }

            return pkt;
        }
    }
    return QByteArray();
}

// ===== Response validation =====

bool SyntecFrameBuilder::checkFrame(const QByteArray& frame, quint8 expectedTid)
{
    if (frame.size() < 20) return false;

    // Check total length field
    qint32 totalLen = 0;
    memcpy(&totalLen, frame.constData(), 4);
    if (frame.size() != totalLen + 4) return false;

    // Check tid
    if (static_cast<quint8>(frame[10]) != expectedTid) return false;

    // Check error codes at offset 12 and 16
    qint32 err1 = 0, err2 = 0;
    memcpy(&err1, frame.constData() + 12, 4);
    memcpy(&err2, frame.constData() + 16, 4);
    if (err1 != 0 || err2 != 0) return false;

    return true;
}

quint8 SyntecFrameBuilder::extractTid(const QByteArray& frame)
{
    if (frame.size() > 10)
        return static_cast<quint8>(frame[10]);
    return 0;
}

// ===== Data extraction =====

qint32 SyntecFrameBuilder::extractInt32(const QByteArray& frame, int offset)
{
    if (frame.size() < offset + 4) return 0;
    qint32 val = 0;
    memcpy(&val, frame.constData() + offset, 4);
    return val;
}

qint64 SyntecFrameBuilder::extractInt64(const QByteArray& frame, int offset)
{
    if (frame.size() < offset + 8) return 0;
    qint64 val = 0;
    memcpy(&val, frame.constData() + offset, 8);
    return val;
}

QString SyntecFrameBuilder::extractString(const QByteArray& frame, int offset)
{
    // Read every other byte (Unicode-like encoding) until 0x00
    QString result;
    int i = offset;
    while (i + 1 < frame.size()) {
        char ch = frame[i];
        if (ch == 0) break;
        result += QChar(static_cast<ushort>(static_cast<quint8>(ch)));
        i += 2;
    }
    return result;
}

// ===== State mapping =====

QString SyntecFrameBuilder::statusToString(int status)
{
    switch (status) {
    case 0: return QStringLiteral("未就绪");
    case 1: return QStringLiteral("就绪");
    case 2: return QStringLiteral("运行中");
    case 3: return QStringLiteral("暂停");
    case 4: return QStringLiteral("停止");
    default: return QString::number(status);
    }
}

QString SyntecFrameBuilder::modeToString(int mode)
{
    switch (mode) {
    case 0: return QStringLiteral("NULL");
    case 1: return QStringLiteral("编辑");
    case 2: return QStringLiteral("自动");
    case 3: return QStringLiteral("MDI");
    case 4: return QStringLiteral("手动");
    case 5: return QStringLiteral("增量");
    case 6: return QStringLiteral("手轮");
    case 7: return QStringLiteral("回零");
    default: return QString::number(mode);
    }
}

QString SyntecFrameBuilder::emgToString(int emg)
{
    return (emg == 0xFF) ? QStringLiteral("急停") : QStringLiteral("正常");
}

QString SyntecFrameBuilder::formatTime(int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

// ===== Empty response =====

bool SyntecFrameBuilder::isEmptyResponse(const QByteArray& data)
{
    if (data.size() < 12) return false;

    // 原始模式：00 00 00 00 FF FF 00 00 00 00 00 00
    // 更灵活的匹配方式：
    // 1. 前两个字节为空（长度字段可能不同）
    // 2. 位置 10 的 cmdCode = 0xFF（keepalive 标识）
    // 或者：
    // 3. 完全匹配原始模式（向后兼容）

    // 检查 cmdCode = 0xFF（位置 10）
    if (static_cast<quint8>(data[10]) == 0xFF) {
        // 检查前 4 字节是否为 00（长度字段可能指示空响应）
        bool first4Zero = (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0);
        return first4Zero;
    }

    // 向后兼容：完全匹配原始模式
    static const char pattern[] = {0x00, 0x00, 0x00, 0x00,
                                    static_cast<char>(0xFF), static_cast<char>(0xFF),
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return memcmp(data.constData(), pattern, 12) == 0;
}
