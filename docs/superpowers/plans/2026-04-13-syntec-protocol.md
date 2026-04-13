# Syntec (新代) CNC Protocol Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate the Syntec CNC binary TCP protocol into the multi-protocol debug tool, with RealtimeWidget, CommandWidget, and ParseWidget.

**Architecture:** Transport layer (TCP) + SyntecProtocol (IProtocol) + SyntecFrameBuilder (static packet encoder/decoder). Sequential polling with tid-based request/response matching. Handshake before polling starts.

**Tech Stack:** Qt 5.15.2, C++14, MSVC (VS 2026 v18), qmake build system

---

## File Structure

```
src/protocol/syntec/
├── syntecframebuilder.h      # Packet templates, response parsing, state mapping
├── syntecframebuilder.cpp
├── syntecprotocol.h           # IProtocol: buildRequest, parseResponse, extractFrame, mock
├── syntecprotocol.cpp
├── syntecwidgetfactory.h      # Creates 3 widgets
├── syntecwidgetfactory.cpp
├── syntecrealtimewidget.h     # Polling state machine, 3+1 layout
├── syntecrealtimewidget.cpp
├── synteccommandwidget.h      # Single command sender
├── synteccommandwidget.cpp
├── syntecparsewidget.h        # Hex frame parser
└── syntecparsewidget.cpp

Modifications:
├── GSK988Tool.pro             # Add 12 Syntec files
├── src/mainwindow.h           # Add forward declarations
└── src/mainwindow.cpp         # Add includes, combo item, case 6, connections
```

---

### Task 1: SyntecFrameBuilder

**Files:**
- Create: `src/protocol/syntec/syntecframebuilder.h`
- Create: `src/protocol/syntec/syntecframebuilder.cpp`

- [ ] **Step 1: Create syntecframebuilder.h**

```cpp
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
```

- [ ] **Step 2: Create syntecframebuilder.cpp**

```cpp
#include "syntecframebuilder"
#include <cstring>

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
            if (pkt.size() > 10)
                pkt[10] = static_cast<char>(tid);
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
    static const char pattern[] = {0x00, 0x00, 0x00, 0x00,
                                    static_cast<char>(0xFF), static_cast<char>(0xFF),
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return data.size() >= 12 && memcmp(data.constData(), pattern, 12) == 0;
}
```

- [ ] **Step 3: Commit**

```bash
mkdir -p src/protocol/syntec
git add src/protocol/syntec/syntecframebuilder.h src/protocol/syntec/syntecframebuilder.cpp
git commit -m "feat(syntec): add SyntecFrameBuilder with packet templates and parsing"
```

---

### Task 2: SyntecProtocol

**Files:**
- Create: `src/protocol/syntec/syntecprotocol.h`
- Create: `src/protocol/syntec/syntecprotocol.cpp`

- [ ] **Step 1: Create syntecprotocol.h**

```cpp
#ifndef SYNTECPROTOCOL_H
#define SYNTECPROTOCOL_H

#include "protocol/iprotocol.h"

class SyntecProtocol : public IProtocol
{
    Q_OBJECT
public:
    explicit SyntecProtocol(QObject* parent = nullptr);

    QString name() const override { return QStringLiteral("Syntec"); }
    QString version() const override { return QStringLiteral("1.0"); }

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;
    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    // Handshake
    QByteArray buildHandshake();

    // TID
    quint8 nextTid() { return m_tid++; }

private:
    quint8 m_tid = 0;
    quint8 m_pendingTid = 0;
    quint8 m_pendingCmdCode = 0;
};

#endif // SYNTECPROTOCOL_H
```

- [ ] **Step 2: Create syntecprotocol.cpp**

```cpp
#include "syntecprotocol.h"
#include "syntecframebuilder.h"
#include <QByteArray>
#include <cstring>

static const QVector<ProtocolCommand> s_commands = {
    {0x00, "握手",         "连接", "无", "握手"},
    {0x01, "程序名",       "状态", "无", "当前程序名"},
    {0x02, "操作模式",     "状态", "无", "0=NULL..7=回零"},
    {0x03, "运行状态",     "状态", "无", "0=未就绪..4=停止"},
    {0x04, "报警数量",     "状态", "无", "报警个数"},
    {0x05, "急停状态",     "状态", "无", "0xFF=急停"},
    {0x06, "开机时间",     "时间", "无", "秒"},
    {0x07, "切削时间",     "时间", "无", "秒"},
    {0x08, "循环时间",     "时间", "无", "秒"},
    {0x09, "工件数",       "计数", "无", "当前工件数"},
    {0x0A, "需求工件数",   "计数", "无", "目标工件数"},
    {0x0B, "总工件数",     "计数", "无", "累计总工件数"},
    {0x0C, "进给倍率",     "倍率", "无", "百分比"},
    {0x0D, "主轴倍率",     "倍率", "无", "百分比"},
    {0x0E, "主轴速度",     "速度", "无", "RPM"},
    {0x0F, "进给速度(原始)", "速度", "无", "需除以10^DecPoint"},
    {0x10, "小数位数",     "速度", "无", "DecPoint"},
    {0x20, "相对坐标",     "坐标", "无", "3轴int64/10^DecPoint"},
    {0x21, "机床坐标",     "坐标", "无", "3轴int64/10^DecPoint"},
};

SyntecProtocol::SyntecProtocol(QObject* parent)
    : IProtocol(parent)
{
}

QByteArray SyntecProtocol::buildHandshake()
{
    return buildRequest(0x00);
}

QByteArray SyntecProtocol::buildRequest(quint8 cmdCode, const QByteArray& params)
{
    Q_UNUSED(params);
    quint8 tid = nextTid();
    m_pendingTid = tid;
    m_pendingCmdCode = cmdCode;
    return SyntecFrameBuilder::buildPacket(cmdCode, tid);
}

ParsedResponse SyntecProtocol::parseResponse(const QByteArray& frame)
{
    ParsedResponse resp;
    if (frame.size() < 20) {
        resp.isValid = false;
        return resp;
    }

    quint8 respTid = SyntecFrameBuilder::extractTid(frame);
    if (respTid != m_pendingTid) {
        resp.isValid = false;
        return resp;
    }

    resp.cmdCode = m_pendingCmdCode;
    resp.isValid = SyntecFrameBuilder::checkFrame(frame, m_pendingTid);
    resp.rawData = frame.mid(20);
    return resp;
}

QByteArray SyntecProtocol::extractFrame(QByteArray& buffer)
{
    // Skip empty/keepalive responses
    while (buffer.size() >= 12 && SyntecFrameBuilder::isEmptyResponse(buffer)) {
        buffer.remove(0, 12);
    }

    if (buffer.size() < 4) return QByteArray();

    qint32 totalLen = 0;
    memcpy(&totalLen, buffer.constData(), 4);
    quint32 frameSize = static_cast<quint32>(totalLen) + 4;

    if (static_cast<quint32>(buffer.size()) < frameSize)
        return QByteArray();

    QByteArray frame = buffer.left(frameSize);
    buffer.remove(0, frameSize);
    return frame;
}

QVector<ProtocolCommand> SyntecProtocol::allCommands() const
{
    return s_commands;
}

ProtocolCommand SyntecProtocol::commandDef(quint8 cmdCode) const
{
    for (const auto& cmd : s_commands) {
        if (cmd.cmdCode == cmdCode) return cmd;
    }
    return ProtocolCommand{cmdCode, QStringLiteral("未知命令"), "", "", ""};
}

QString SyntecProtocol::interpretData(quint8 cmdCode, const QByteArray& data) const
{
    using FB = SyntecFrameBuilder;
    switch (cmdCode) {
    case 0x01: return QStringLiteral("程序名: %1").arg(FB::extractString(data, 0));
    case 0x02: return QStringLiteral("操作模式: %1 (%2)").arg(FB::modeToString(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x03: return QStringLiteral("运行状态: %1 (%2)").arg(FB::statusToString(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x04: return QStringLiteral("报警数量: %1").arg(FB::extractInt32(data, 0));
    case 0x05: {
        if (data.size() >= 8) {
            int emg = static_cast<quint8>(data[4]);
            return QStringLiteral("急停: %1 (0x%2)").arg(FB::emgToString(emg)).arg(emg, 2, 16, QChar('0'));
        }
        break;
    }
    case 0x06: return QStringLiteral("开机时间: %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x07: return QStringLiteral("切削时间: %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x08: return QStringLiteral("循环时间: %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    case 0x09: return QStringLiteral("工件数: %1").arg(FB::extractInt32(data, 0));
    case 0x0A: return QStringLiteral("需求工件数: %1").arg(FB::extractInt32(data, 0));
    case 0x0B: return QStringLiteral("总工件数: %1").arg(FB::extractInt32(data, 0));
    case 0x0C: return QStringLiteral("进给倍率: %1%").arg(FB::extractInt32(data, 0));
    case 0x0D: return QStringLiteral("主轴倍率: %1%").arg(FB::extractInt32(data, 0));
    case 0x0E: return QStringLiteral("主轴速度: %1 RPM").arg(FB::extractInt32(data, 0));
    case 0x0F: return QStringLiteral("进给速度(原始): %1").arg(FB::extractInt32(data, 0));
    case 0x10: return QStringLiteral("小数位数: %1").arg(FB::extractInt32(data, 0));
    case 0x20:
    case 0x21: {
        QString label = (cmdCode == 0x20) ? QStringLiteral("相对") : QStringLiteral("机床");
        QString result = label + QStringLiteral("坐标: ");
        for (int i = 0; i < 3; ++i) {
            qint64 raw = FB::extractInt64(data, i * 8);
            if (i > 0) result += ", ";
            result += QString::number(raw);
        }
        return result;
    }
    default:
        break;
    }

    // Fallback: hex dump
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
    return QStringLiteral("HEX: %1").arg(hex.join(" "));
}

QByteArray SyntecProtocol::mockResponse(quint8 cmdCode, const QByteArray& requestData) const
{
    // Extract tid from request
    quint8 tid = (requestData.size() > 10) ? static_cast<quint8>(requestData[10]) : 0;

    // Build response: [4B totalLen][4B vendor=0xC8][2B vendor2=0xC8][1B tid][1B 0x00][4B err1=0][4B err2=0][data...]
    auto buildResp = [](quint8 t, const QByteArray& data) -> QByteArray {
        QByteArray resp;
        qint32 totalLen = 16 + data.size();
        resp.resize(20 + data.size());
        memset(resp.data(), 0, resp.size());
        memcpy(resp.data(), &totalLen, 4);
        qint32 vendor = 0xC8;
        memcpy(resp.data() + 4, &vendor, 4);
        resp[8] = static_cast<char>(0xC8);
        resp[9] = 0;
        resp[10] = static_cast<char>(t);
        resp[11] = 0;
        // err1 and err2 already 0
        memcpy(resp.data() + 20, data.constData(), data.size());
        return resp;
    };

    QByteArray mockData;
    switch (cmdCode) {
    case 0x00: // Handshake - empty data
        break;
    case 0x01: { // ProgramName - encoded string "O1234"
        const char* str = "O1234";
        mockData.resize(20);
        for (int i = 0; str[i]; ++i) {
            mockData[i * 2] = str[i];
        }
        break;
    }
    case 0x02: { // Mode = AUTO(2)
        qint32 v = 2;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x03: { // Status = START(2)
        qint32 v = 2;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x04: { // Alarm = 0
        qint32 v = 0;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x05: { // EMG = normal (not 0xFF)
        mockData.append(8, '\0');
        mockData[4] = 0x00; // not emergency
        break;
    }
    case 0x06: // RunTime = 3600s
    case 0x07: // CutTime = 1800s
    case 0x08: { // CycleTime = 120s
        qint32 v = (cmdCode == 0x06) ? 3600 : (cmdCode == 0x07) ? 1800 : 120;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x09: // Products = 5
    case 0x0A: // RequiredProducts = 100
    case 0x0B: { // TotalProducts = 50
        qint32 v = (cmdCode == 0x09) ? 5 : (cmdCode == 0x0A) ? 100 : 50;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x0C: // FeedOverride = 100%
    case 0x0D: { // SpindleOverride = 80%
        qint32 v = (cmdCode == 0x0C) ? 100 : 80;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x0E: { // SpindleSpeed = 3000 RPM
        qint32 v = 3000;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x0F: { // FeedRateOri = 12000
        qint32 v = 12000;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x10: { // DecPoint = 3
        qint32 v = 3;
        mockData.append(reinterpret_cast<const char*>(&v), 4);
        mockData.append(4, '\0');
        break;
    }
    case 0x20: // RelativeAxes
    case 0x21: { // MachineAxes - 3 axes, each 8 bytes int64
        // Values: X=100000, Y=200000, Z=-50000 (in 10^-3 units = 100.000, 200.000, -50.000)
        for (int i = 0; i < 3; ++i) {
            qint64 val = (i == 0) ? 100000 : (i == 1) ? 200000 : -50000;
            mockData.append(reinterpret_cast<const char*>(&val), 8);
        }
        break;
    }
    default:
        mockData.append(8, '\0');
        break;
    }

    return buildResp(tid, mockData);
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/syntec/syntecprotocol.h src/protocol/syntec/syntecprotocol.cpp
git commit -m "feat(syntec): add SyntecProtocol with IProtocol implementation and mock responses"
```

---

### Task 3: SyntecWidgetFactory + SyntecParseWidget

**Files:**
- Create: `src/protocol/syntec/syntecwidgetfactory.h`
- Create: `src/protocol/syntec/syntecwidgetfactory.cpp`
- Create: `src/protocol/syntec/syntecparsewidget.h`
- Create: `src/protocol/syntec/syntecparsewidget.cpp`

- [ ] **Step 1: Create syntecwidgetfactory.h**

```cpp
#ifndef SYNTECWIDGETFACTORY_H
#define SYNTECWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class SyntecWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // SYNTECWIDGETFACTORY_H
```

- [ ] **Step 2: Create syntecwidgetfactory.cpp**

```cpp
#include "syntecwidgetfactory.h"
#include "syntecrealtimewidget.h"
#include "synteccommandwidget.h"
#include "syntecparsewidget.h"

QWidget* SyntecWidgetFactory::createRealtimeWidget()
{
    return new SyntecRealtimeWidget;
}

QWidget* SyntecWidgetFactory::createCommandWidget()
{
    return new SyntecCommandWidget;
}

QWidget* SyntecWidgetFactory::createParseWidget()
{
    return new SyntecParseWidget;
}
```

- [ ] **Step 3: Create syntecparsewidget.h**

```cpp
#ifndef SYNTECPARSEWIDGET_H
#define SYNTECPARSEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>

class SyntecParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyntecParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QComboBox* m_typeCombo;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;
};

#endif // SYNTECPARSEWIDGET_H
```

- [ ] **Step 4: Create syntecparsewidget.cpp**

```cpp
#include "syntecparsewidget.h"
#include "syntecframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

SyntecParseWidget::SyntecParseWidget(QWidget* parent) : QWidget(parent) { setupUI(); }

void SyntecParseWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(QStringLiteral("输入Hex帧 (空格分隔):")));
    m_hexInput = new QTextEdit;
    m_hexInput->setMaximumHeight(60);
    m_hexInput->setFontFamily("Consolas");
    layout->addWidget(m_hexInput);

    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel(QStringLiteral("数据类型:")));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({
        QStringLiteral("自动"), QStringLiteral("运行状态"), QStringLiteral("操作模式"),
        QStringLiteral("报警"), QStringLiteral("急停"), QStringLiteral("时间(秒)"),
        QStringLiteral("工件数"), QStringLiteral("倍率(%)"), QStringLiteral("速度(RPM)"),
        QStringLiteral("坐标(int64)"), QStringLiteral("程序名"), QStringLiteral("小数位数"),
        QStringLiteral("原始HEX")
    });
    ctrlLayout->addWidget(m_typeCombo);
    m_parseBtn = new QPushButton(QStringLiteral("解析"));
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &SyntecParseWidget::doParse);
    ctrlLayout->addWidget(m_parseBtn);
    ctrlLayout->addStretch();
    layout->addLayout(ctrlLayout);

    layout->addWidget(new QLabel(QStringLiteral("解析结果:")));
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");
    layout->addWidget(m_resultDisplay, 1);
}

void SyntecParseWidget::doParse()
{
    using FB = SyntecFrameBuilder;

    QString hexStr = m_hexInput->toPlainText().trimmed();
    hexStr.remove(' ');
    hexStr.remove(',');
    hexStr.remove("0x", Qt::CaseInsensitive);
    QByteArray frame = QByteArray::fromHex(hexStr.toUtf8());

    if (frame.size() < 4) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧太短</span>");
        return;
    }

    qint32 totalLen = 0;
    memcpy(&totalLen, frame.constData(), 4);
    quint8 tid = FB::extractTid(frame);
    qint32 err1 = FB::extractInt32(frame, 12);
    qint32 err2 = FB::extractInt32(frame, 16);

    QString html;
    html += QStringLiteral("<b>帧长度:</b> %1 字节<br>").arg(frame.size());
    html += QStringLiteral("<b>数据长度:</b> %1<br>").arg(totalLen);
    html += QStringLiteral("<b>TID:</b> %1<br>").arg(tid);
    html += QStringLiteral("<b>错误码1:</b> %1, <b>错误码2:</b> %2<br>").arg(err1).arg(err2);

    if (err1 != 0 || err2 != 0) {
        html += QStringLiteral("<span style='color:red;'>帧包含错误!</span><br>");
    }

    if (frame.size() < 20) {
        html += QStringLiteral("<span style='color:orange;'>帧不包含数据区</span>");
        m_resultDisplay->setHtml(html);
        return;
    }

    QByteArray data = frame.mid(20);
    int type = m_typeCombo->currentIndex();

    // Auto-detect or use selected type
    if (type == 0) {
        // Auto: try to show as hex + int values
        QStringList hexList;
        for (int i = 0; i < data.size(); ++i)
            hexList << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        html += QStringLiteral("<br><b>数据区 HEX:</b> %1<br>").arg(hexList.join(" "));

        if (data.size() >= 4) {
            html += QStringLiteral("<b>Int32[0]:</b> %1<br>").arg(FB::extractInt32(data, 0));
        }
        if (data.size() >= 8) {
            html += QStringLiteral("<b>Int32[4]:</b> %1<br>").arg(FB::extractInt32(data, 4));
        }
        if (data.size() >= 12) {
            html += QStringLiteral("<b>Int32[8]:</b> %1<br>").arg(FB::extractInt32(data, 8));
        }
    } else if (type == 1) { // 运行状态
        html += QStringLiteral("<b>运行状态:</b> %1 (%2)")
            .arg(FB::statusToString(FB::extractInt32(data, 0)))
            .arg(FB::extractInt32(data, 0));
    } else if (type == 2) { // 操作模式
        html += QStringLiteral("<b>操作模式:</b> %1 (%2)")
            .arg(FB::modeToString(FB::extractInt32(data, 0)))
            .arg(FB::extractInt32(data, 0));
    } else if (type == 3) { // 报警
        html += QStringLiteral("<b>报警数量:</b> %1").arg(FB::extractInt32(data, 0));
    } else if (type == 4) { // 急停
        if (data.size() >= 5) {
            int emg = static_cast<quint8>(data[4]);
            html += QStringLiteral("<b>急停:</b> %1 (0x%2)")
                .arg(FB::emgToString(emg)).arg(emg, 2, 16, QChar('0'));
        }
    } else if (type == 5) { // 时间
        html += QStringLiteral("<b>时间:</b> %1 (%2秒)")
            .arg(FB::formatTime(FB::extractInt32(data, 0)))
            .arg(FB::extractInt32(data, 0));
    } else if (type == 6) { // 工件数
        html += QStringLiteral("<b>工件数:</b> %1").arg(FB::extractInt32(data, 0));
    } else if (type == 7) { // 倍率
        html += QStringLiteral("<b>倍率:</b> %1%").arg(FB::extractInt32(data, 0));
    } else if (type == 8) { // 速度
        html += QStringLiteral("<b>速度:</b> %1 RPM").arg(FB::extractInt32(data, 0));
    } else if (type == 9) { // 坐标
        for (int i = 0; i < 3 && i * 8 + 8 <= data.size(); ++i) {
            qint64 raw = FB::extractInt64(data, i * 8);
            char axis = 'X' + i;
            html += QStringLiteral("<b>%1:</b> %2 (raw)<br>").arg(axis).arg(raw);
        }
    } else if (type == 10) { // 程序名
        html += QStringLiteral("<b>程序名:</b> %1").arg(FB::extractString(data, 0));
    } else if (type == 11) { // 小数位数
        html += QStringLiteral("<b>小数位数:</b> %1").arg(FB::extractInt32(data, 0));
    } else { // Raw HEX
        QStringList hexList;
        for (int i = 0; i < data.size(); ++i)
            hexList << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        html += QStringLiteral("<b>HEX:</b> %1").arg(hexList.join(" "));
    }

    m_resultDisplay->setHtml(html);
}
```

- [ ] **Step 5: Commit**

```bash
git add src/protocol/syntec/syntecwidgetfactory.h src/protocol/syntec/syntecwidgetfactory.cpp src/protocol/syntec/syntecparsewidget.h src/protocol/syntec/syntecparsewidget.cpp
git commit -m "feat(syntec): add SyntecWidgetFactory and SyntecParseWidget"
```

---

### Task 4: SyntecCommandWidget

**Files:**
- Create: `src/protocol/syntec/synteccommandwidget.h`
- Create: `src/protocol/syntec/synteccommandwidget.cpp`

- [ ] **Step 1: Create synteccommandwidget.h**

```cpp
#ifndef SYNTECCOMMANDWIDGET_H
#define SYNTECCOMMANDWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include "protocol/iprotocol.h"

class SyntecCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyntecCommandWidget(QWidget* parent = nullptr);

    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();

    QComboBox* m_cmdCombo;
    QPushButton* m_sendBtn;
    QTextEdit* m_resultDisplay;
};

#endif // SYNTECCOMMANDWIDGET_H
```

- [ ] **Step 2: Create synteccommandwidget.cpp**

```cpp
#include "synteccommandwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFormLayout>

SyntecCommandWidget::SyntecCommandWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SyntecCommandWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    // Tab widget for organization
    auto* tabs = new QTabWidget;
    layout->addWidget(tabs);

    // === Tab 1: 状态查询 ===
    {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);

        auto* ctrlLayout = new QHBoxLayout;
        ctrlLayout->addWidget(new QLabel(QStringLiteral("命令:")));
        m_cmdCombo = new QComboBox;
        // Populate with non-handshake commands
        m_cmdCombo->addItem(QStringLiteral("程序名 (0x01)"), 0x01);
        m_cmdCombo->addItem(QStringLiteral("操作模式 (0x02)"), 0x02);
        m_cmdCombo->addItem(QStringLiteral("运行状态 (0x03)"), 0x03);
        m_cmdCombo->addItem(QStringLiteral("报警数量 (0x04)"), 0x04);
        m_cmdCombo->addItem(QStringLiteral("急停状态 (0x05)"), 0x05);
        m_cmdCombo->addItem(QStringLiteral("开机时间 (0x06)"), 0x06);
        m_cmdCombo->addItem(QStringLiteral("切削时间 (0x07)"), 0x07);
        m_cmdCombo->addItem(QStringLiteral("循环时间 (0x08)"), 0x08);
        m_cmdCombo->addItem(QStringLiteral("工件数 (0x09)"), 0x09);
        m_cmdCombo->addItem(QStringLiteral("需求工件数 (0x0A)"), 0x0A);
        m_cmdCombo->addItem(QStringLiteral("总工件数 (0x0B)"), 0x0B);
        m_cmdCombo->addItem(QStringLiteral("进给倍率 (0x0C)"), 0x0C);
        m_cmdCombo->addItem(QStringLiteral("主轴倍率 (0x0D)"), 0x0D);
        m_cmdCombo->addItem(QStringLiteral("主轴速度 (0x0E)"), 0x0E);
        m_cmdCombo->addItem(QStringLiteral("进给速度 (0x0F)"), 0x0F);
        m_cmdCombo->addItem(QStringLiteral("小数位数 (0x10)"), 0x10);
        m_cmdCombo->addItem(QStringLiteral("相对坐标 (0x20)"), 0x20);
        m_cmdCombo->addItem(QStringLiteral("机床坐标 (0x21)"), 0x21);
        ctrlLayout->addWidget(m_cmdCombo);

        m_sendBtn = new QPushButton(QStringLiteral("查询"));
        m_sendBtn->setFixedWidth(80);
        ctrlLayout->addWidget(m_sendBtn);
        ctrlLayout->addStretch();
        l->addLayout(ctrlLayout);

        m_resultDisplay = new QTextEdit;
        m_resultDisplay->setReadOnly(true);
        m_resultDisplay->setFontFamily("Consolas");
        m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");
        l->addWidget(m_resultDisplay);

        connect(m_sendBtn, &QPushButton::clicked, this, [this]() {
            int idx = m_cmdCombo->currentIndex();
            if (idx < 0) return;
            quint8 cmdCode = static_cast<quint8>(m_cmdCombo->itemData(idx).toInt());
            m_resultDisplay->clear();
            emit sendCommand(cmdCode, QByteArray());
        });

        tabs->addTab(w, QStringLiteral("状态查询"));
    }

    // === Tab 2: 操作 ===
    {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        l->addWidget(new QLabel(QStringLiteral("新代协议暂不支持写入命令。\n后续有写入协议文档可扩展。")));
        l->addStretch();
        tabs->addTab(w, QStringLiteral("操作"));
    }
}

void SyntecCommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    QStringList hex;
    for (int i = 0; i < resp.rawData.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(resp.rawData[i]), 2, 16, QChar('0')).toUpper();

    m_resultDisplay->append(QStringLiteral("=== 响应 ==="));
    m_resultDisplay->append(QStringLiteral("状态: %1").arg(resp.isValid ? QStringLiteral("成功") : QStringLiteral("失败")));
    m_resultDisplay->append(QStringLiteral("解析: %1").arg(interpretation));
    m_resultDisplay->append(QStringLiteral("HEX: %1").arg(hex.join(" ")));
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/syntec/synteccommandwidget.h src/protocol/syntec/synteccommandwidget.cpp
git commit -m "feat(syntec): add SyntecCommandWidget with status query and operation tabs"
```

---

### Task 5: SyntecRealtimeWidget

**Files:**
- Create: `src/protocol/syntec/syntecrealtimewidget.h`
- Create: `src/protocol/syntec/syntecrealtimewidget.cpp`

This is the largest component. It implements the polling state machine with 16 poll items and a 3+1 row layout.

- [ ] **Step 1: Create syntecrealtimewidget.h**

```cpp
#ifndef SYNTECREALTIMEWIDGET_H
#define SYNTECREALTIMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include "protocol/iprotocol.h"

class SyntecRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyntecRealtimeWidget(QWidget* parent = nullptr);

    void updateData(const ParsedResponse& resp);
    void startPolling();
    void stopPolling();
    void clearData();

public slots:
    void appendHexDisplay(const QByteArray& data, bool isSend);

signals:
    void pollRequest(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void startCycle();
    void setLabelOK(QLabel* label, const QString& text);
    void setLabelError(QLabel* label, const QString& err);
    void resetLabel(QLabel* label);

    // Controls
    QCheckBox* m_autoRefreshCheck;
    QComboBox* m_intervalCombo;
    QPushButton* m_manualRefreshBtn;
    QTextEdit* m_hexDisplay;

    // Status group
    QGroupBox* m_statusGroup;
    QLabel* m_statusLabel;
    QLabel* m_modeLabel;
    QLabel* m_progNameLabel;
    QLabel* m_emgLabel;
    QLabel* m_alarmLabel;
    QLabel* m_productsLabel;

    // Feed/Spindle group
    QGroupBox* m_feedGroup;
    QLabel* m_feedRateLabel;
    QLabel* m_feedOverrideLabel;
    QLabel* m_spindleSpeedLabel;
    QLabel* m_spindleOverrideLabel;

    // Coordinate group
    QGroupBox* m_coordGroup;
    QLabel* m_machCoord[3];
    QLabel* m_relCoord[3];

    // Time group
    QGroupBox* m_timeGroup;
    QLabel* m_runTimeLabel;
    QLabel* m_cutTimeLabel;
    QLabel* m_cycleTimeLabel;

    // Polling state
    QTimer* m_cycleDelayTimer;
    QTimer* m_interPollTimer;
    int m_pollIndex;
    bool m_cycleActive;
    int m_decPoint; // cached decimal point value

    enum PollIndex {
        PI_STATUS = 0,         // 0x03
        PI_MODE,               // 0x02
        PI_PROGNAME,           // 0x01
        PI_ALARM,              // 0x04
        PI_EMG,                // 0x05
        PI_DECPOINT,           // 0x10 (must be before coordinates/feed)
        PI_REL_COORD,          // 0x20
        PI_MACH_COORD,         // 0x21
        PI_FEED_OVERRIDE,      // 0x0C
        PI_SPINDLE_OVERRIDE,   // 0x0D
        PI_SPINDLE_SPEED,      // 0x0E
        PI_FEED_RATE_ORI,      // 0x0F
        PI_RUN_TIME,           // 0x06
        PI_CUT_TIME,           // 0x07
        PI_CYCLE_TIME,         // 0x08
        PI_PRODUCTS,           // 0x09
        PI_COUNT
    };

    struct PollItem {
        quint8 cmdCode;
        QByteArray params;
    };
    static const PollItem pollItems[];
};

#endif // SYNTECREALTIMEWIDGET_H
```

- [ ] **Step 2: Create syntecrealtimewidget.cpp**

```cpp
#include "syntecrealtimewidget.h"
#include "syntecframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QFrame>
#include <cstring>

// ========== Shared Styles ==========

static const char* GROUP_STYLE =
    "QGroupBox {"
    "  font-weight: bold;"
    "  font-size: 12px;"
    "  border: 1px solid #c8c8c8;"
    "  border-radius: 6px;"
    "  margin-top: 12px;"
    "  padding: 10px 6px 6px 6px;"
    "  background: #ffffff;"
    "}"
    "QGroupBox::title {"
    "  subcontrol-origin: margin;"
    "  subcontrol-position: top left;"
    "  padding: 2px 8px;"
    "  color: #333;"
    "}";

static const char* VALUE_STYLE =
    "font-size: 13px; font-weight: bold; font-family: Consolas, 'Courier New', monospace;"
    "color: #1a1a1a; padding: 2px 6px;"
    "background: #f5f6f8; border: 1px solid #dcdfe3; border-radius: 3px;";

static const char* COORD_HEADER_STYLE =
    "font-weight: bold; font-size: 12px; color: #666; padding: 2px 4px;";

static const char* COORD_ROW_STYLE =
    "font-size: 12px; color: #555; padding: 2px 4px;";

// ========== Poll Items ==========

const SyntecRealtimeWidget::PollItem SyntecRealtimeWidget::pollItems[] = {
    {0x03, QByteArray()}, // PI_STATUS
    {0x02, QByteArray()}, // PI_MODE
    {0x01, QByteArray()}, // PI_PROGNAME
    {0x04, QByteArray()}, // PI_ALARM
    {0x05, QByteArray()}, // PI_EMG
    {0x10, QByteArray()}, // PI_DECPOINT
    {0x20, QByteArray()}, // PI_REL_COORD
    {0x21, QByteArray()}, // PI_MACH_COORD
    {0x0C, QByteArray()}, // PI_FEED_OVERRIDE
    {0x0D, QByteArray()}, // PI_SPINDLE_OVERRIDE
    {0x0E, QByteArray()}, // PI_SPINDLE_SPEED
    {0x0F, QByteArray()}, // PI_FEED_RATE_ORI
    {0x06, QByteArray()}, // PI_RUN_TIME
    {0x07, QByteArray()}, // PI_CUT_TIME
    {0x08, QByteArray()}, // PI_CYCLE_TIME
    {0x09, QByteArray()}, // PI_PRODUCTS
};

// ========== Helpers ==========

static QLabel* makeValueLabel()
{
    auto* lbl = new QLabel("--");
    lbl->setStyleSheet(VALUE_STYLE);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl->setFixedHeight(22);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return lbl;
}

static QLabel* makeRowLabel(const QString& text)
{
    auto* lbl = new QLabel(text);
    lbl->setStyleSheet(COORD_ROW_STYLE);
    return lbl;
}

static QLabel* makeHeaderLabel(const QString& text)
{
    auto* lbl = new QLabel(text);
    lbl->setStyleSheet(COORD_HEADER_STYLE);
    lbl->setAlignment(Qt::AlignCenter);
    return lbl;
}

// ========== Constructor ==========

SyntecRealtimeWidget::SyntecRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_cycleActive(false)
    , m_decPoint(3) // default 3 decimal places
{
    setupUI();

    m_cycleDelayTimer = new QTimer(this);
    m_cycleDelayTimer->setSingleShot(true);
    connect(m_cycleDelayTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRefreshCheck->isChecked())
            startCycle();
    });

    // Inter-poll delay
    m_interPollTimer = new QTimer(this);
    m_interPollTimer->setSingleShot(true);
    m_interPollTimer->setInterval(100);
    connect(m_interPollTimer, &QTimer::timeout, this, [this]() {
        if (!m_cycleActive) return;
        if (m_pollIndex >= PI_COUNT) {
            m_cycleActive = false;
            if (m_autoRefreshCheck->isChecked()) {
                int secs = m_intervalCombo->currentData().toInt();
                m_cycleDelayTimer->start(secs * 1000);
            }
        } else {
            const auto& item = pollItems[m_pollIndex];
            emit pollRequest(item.cmdCode, item.params);
        }
    });

    connect(m_manualRefreshBtn, &QPushButton::clicked, this, [this]() {
        startCycle();
    });

    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_intervalCombo->setEnabled(checked);
        if (!checked) {
            m_cycleDelayTimer->stop();
            m_interPollTimer->stop();
        }
    });
}

void SyntecRealtimeWidget::setLabelOK(QLabel* label, const QString& text)
{
    label->setText(text);
    label->setStyleSheet(VALUE_STYLE);
}

void SyntecRealtimeWidget::setLabelError(QLabel* label, const QString&)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

void SyntecRealtimeWidget::resetLabel(QLabel* label)
{
    label->setText("--");
    label->setStyleSheet(VALUE_STYLE);
}

// ========== UI Setup ==========

void SyntecRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);

    // === Control bar ===
    auto* controlBar = new QHBoxLayout;
    controlBar->setSpacing(8);
    m_autoRefreshCheck = new QCheckBox(QStringLiteral("自动刷新"));
    m_autoRefreshCheck->setChecked(true);
    controlBar->addWidget(m_autoRefreshCheck);
    controlBar->addWidget(new QLabel(QStringLiteral("间隔:")));
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItem(QStringLiteral("0.5 秒"), 1);   // index 0 but 1s for stability
    m_intervalCombo->addItem(QStringLiteral("1 秒"), 1);
    m_intervalCombo->addItem(QStringLiteral("2 秒"), 2);
    m_intervalCombo->addItem(QStringLiteral("5 秒"), 5);
    m_intervalCombo->setCurrentIndex(1);
    controlBar->addWidget(m_intervalCombo);
    m_manualRefreshBtn = new QPushButton(QStringLiteral("手动刷新"));
    m_manualRefreshBtn->setFixedWidth(80);
    controlBar->addWidget(m_manualRefreshBtn);
    controlBar->addStretch();
    mainLayout->addLayout(controlBar);

    // === Scroll area ===
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: #f0f0f0; }");
    auto* scrollWidget = new QWidget;
    scrollWidget->setStyleSheet("background: #f0f0f0;");
    auto* grid = new QGridLayout(scrollWidget);
    grid->setSpacing(8);
    grid->setContentsMargins(6, 6, 6, 6);

    // ===== Row 0: 运行状态 | 进给/主轴 =====

    m_statusGroup = new QGroupBox(QStringLiteral("运行状态"));
    m_statusGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_statusLabel = makeValueLabel();
        m_modeLabel = makeValueLabel();
        m_progNameLabel = makeValueLabel();
        m_emgLabel = makeValueLabel();
        m_alarmLabel = makeValueLabel();
        m_productsLabel = makeValueLabel();
        l->addRow(QStringLiteral("运行状态:"), m_statusLabel);
        l->addRow(QStringLiteral("操作模式:"), m_modeLabel);
        l->addRow(QStringLiteral("程序名:"), m_progNameLabel);
        l->addRow(QStringLiteral("急停:"), m_emgLabel);
        l->addRow(QStringLiteral("报警:"), m_alarmLabel);
        l->addRow(QStringLiteral("工件数:"), m_productsLabel);
        m_statusGroup->setLayout(l);
    }

    m_feedGroup = new QGroupBox(QStringLiteral("进给/主轴"));
    m_feedGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_feedRateLabel = makeValueLabel();
        m_feedOverrideLabel = makeValueLabel();
        m_spindleSpeedLabel = makeValueLabel();
        m_spindleOverrideLabel = makeValueLabel();
        l->addRow(QStringLiteral("进给速度:"), m_feedRateLabel);
        l->addRow(QStringLiteral("进给倍率:"), m_feedOverrideLabel);
        l->addRow(QStringLiteral("主轴速度:"), m_spindleSpeedLabel);
        l->addRow(QStringLiteral("主轴倍率:"), m_spindleOverrideLabel);
        m_feedGroup->setLayout(l);
    }

    m_timeGroup = new QGroupBox(QStringLiteral("时间"));
    m_timeGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QFormLayout;
        l->setSpacing(4);
        m_runTimeLabel = makeValueLabel();
        m_cutTimeLabel = makeValueLabel();
        m_cycleTimeLabel = makeValueLabel();
        l->addRow(QStringLiteral("开机时间:"), m_runTimeLabel);
        l->addRow(QStringLiteral("切削时间:"), m_cutTimeLabel);
        l->addRow(QStringLiteral("循环时间:"), m_cycleTimeLabel);
        m_timeGroup->setLayout(l);
    }

    // ===== Row 1: 坐标表 (spans 3 cols) =====

    m_coordGroup = new QGroupBox(QStringLiteral("坐标"));
    m_coordGroup->setStyleSheet(GROUP_STYLE);
    {
        auto* l = new QGridLayout;
        l->setSpacing(4);
        l->setContentsMargins(8, 16, 8, 8);
        // Header row
        l->addWidget(makeHeaderLabel(""), 0, 0);
        l->addWidget(makeHeaderLabel("X"), 0, 1);
        l->addWidget(makeHeaderLabel("Y"), 0, 2);
        l->addWidget(makeHeaderLabel("Z"), 0, 3);
        // Separator
        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #d0d0d0;");
        l->addWidget(sep, 1, 0, 1, 4);
        // Data rows
        l->addWidget(makeRowLabel(QStringLiteral("机床坐标:")), 2, 0);
        l->addWidget(makeRowLabel(QStringLiteral("相对坐标:")), 3, 0);
        for (int i = 0; i < 3; ++i) {
            m_machCoord[i] = makeValueLabel();
            m_relCoord[i] = makeValueLabel();
            l->addWidget(m_machCoord[i], 2, i + 1);
            l->addWidget(m_relCoord[i], 3, i + 1);
        }
        m_coordGroup->setLayout(l);
    }

    // Grid layout
    grid->addWidget(m_statusGroup,  0, 0);
    grid->addWidget(m_feedGroup,    0, 1);
    grid->addWidget(m_timeGroup,    0, 2);
    grid->addWidget(m_coordGroup,   1, 0, 1, 3);

    for (int c = 0; c < 3; ++c)
        grid->setColumnStretch(c, 1);

    scrollArea->setWidget(scrollWidget);

    // HEX display
    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(80);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");

    mainLayout->addWidget(scrollArea, 3);
    mainLayout->addWidget(m_hexDisplay, 1);
}

// ========== Cycle Polling ==========

void SyntecRealtimeWidget::startCycle()
{
    if (m_cycleActive) return;
    m_cycleActive = true;
    m_pollIndex = 0;
    const auto& item = pollItems[0];
    emit pollRequest(item.cmdCode, item.params);
}

// ========== Update Data ==========

void SyntecRealtimeWidget::updateData(const ParsedResponse& resp)
{
    if (!m_cycleActive) return;

    int currentIdx = m_pollIndex;
    const QByteArray& data = resp.rawData;
    using FB = SyntecFrameBuilder;

    if (resp.isValid) {
        switch (currentIdx) {
        case PI_STATUS: { // 0x03
            if (data.size() >= 4) {
                int status = FB::extractInt32(data, 0);
                setLabelOK(m_statusLabel, FB::statusToString(status));
            }
            break;
        }
        case PI_MODE: { // 0x02
            if (data.size() >= 4) {
                int mode = FB::extractInt32(data, 0);
                setLabelOK(m_modeLabel, FB::modeToString(mode));
            }
            break;
        }
        case PI_PROGNAME: { // 0x01
            setLabelOK(m_progNameLabel, FB::extractString(data, 0));
            break;
        }
        case PI_ALARM: { // 0x04
            if (data.size() >= 4) {
                int alarm = FB::extractInt32(data, 0);
                setLabelOK(m_alarmLabel, alarm > 0 ? QString::number(alarm) : QStringLiteral("无"));
            }
            break;
        }
        case PI_EMG: { // 0x05 - data at offset 4 in response data
            if (data.size() >= 5) {
                int emg = static_cast<quint8>(data[4]);
                setLabelOK(m_emgLabel, FB::emgToString(emg));
            }
            break;
        }
        case PI_DECPOINT: { // 0x10
            if (data.size() >= 4) {
                m_decPoint = FB::extractInt32(data, 0);
                if (m_decPoint < 0) m_decPoint = 3;
            }
            break;
        }
        case PI_REL_COORD: { // 0x20 - 3 axes, int64 each, divide by 10^decPoint
            for (int i = 0; i < 3 && i * 8 + 8 <= data.size(); ++i) {
                qint64 raw = FB::extractInt64(data, i * 8);
                double val = static_cast<double>(raw);
                for (int d = 0; d < m_decPoint; ++d) val /= 10.0;
                setLabelOK(m_relCoord[i], QString::number(val, 'f', qMax(m_decPoint, 1)));
            }
            break;
        }
        case PI_MACH_COORD: { // 0x21
            for (int i = 0; i < 3 && i * 8 + 8 <= data.size(); ++i) {
                qint64 raw = FB::extractInt64(data, i * 8);
                double val = static_cast<double>(raw);
                for (int d = 0; d < m_decPoint; ++d) val /= 10.0;
                setLabelOK(m_machCoord[i], QString::number(val, 'f', qMax(m_decPoint, 1)));
            }
            break;
        }
        case PI_FEED_OVERRIDE: { // 0x0C
            if (data.size() >= 4) {
                setLabelOK(m_feedOverrideLabel, QString::number(FB::extractInt32(data, 0)) + "%");
            }
            break;
        }
        case PI_SPINDLE_OVERRIDE: { // 0x0D
            if (data.size() >= 4) {
                setLabelOK(m_spindleOverrideLabel, QString::number(FB::extractInt32(data, 0)) + "%");
            }
            break;
        }
        case PI_SPINDLE_SPEED: { // 0x0E
            if (data.size() >= 4) {
                setLabelOK(m_spindleSpeedLabel, QString::number(FB::extractInt32(data, 0)) + " RPM");
            }
            break;
        }
        case PI_FEED_RATE_ORI: { // 0x0F - raw value / 10^decPoint
            if (data.size() >= 4) {
                qint32 raw = FB::extractInt32(data, 0);
                double val = static_cast<double>(raw);
                for (int d = 0; d < m_decPoint; ++d) val /= 10.0;
                setLabelOK(m_feedRateLabel, QString::number(val, 'f', qMax(m_decPoint, 1)));
            }
            break;
        }
        case PI_RUN_TIME: { // 0x06
            if (data.size() >= 4) {
                setLabelOK(m_runTimeLabel, FB::formatTime(FB::extractInt32(data, 0)));
            }
            break;
        }
        case PI_CUT_TIME: { // 0x07
            if (data.size() >= 4) {
                setLabelOK(m_cutTimeLabel, FB::formatTime(FB::extractInt32(data, 0)));
            }
            break;
        }
        case PI_CYCLE_TIME: { // 0x08
            if (data.size() >= 4) {
                setLabelOK(m_cycleTimeLabel, FB::formatTime(FB::extractInt32(data, 0)));
            }
            break;
        }
        case PI_PRODUCTS: { // 0x09
            if (data.size() >= 4) {
                setLabelOK(m_productsLabel, QString::number(FB::extractInt32(data, 0)));
            }
            break;
        }
        }
    } else {
        // Invalid response
        switch (currentIdx) {
        case PI_STATUS:     setLabelError(m_statusLabel, QString()); break;
        case PI_MODE:       setLabelError(m_modeLabel, QString()); break;
        case PI_PROGNAME:   setLabelError(m_progNameLabel, QString()); break;
        case PI_ALARM:      setLabelError(m_alarmLabel, QString()); break;
        case PI_EMG:        setLabelError(m_emgLabel, QString()); break;
        case PI_DECPOINT:   break; // silent
        case PI_REL_COORD:  for (int i = 0; i < 3; ++i) setLabelError(m_relCoord[i], QString()); break;
        case PI_MACH_COORD: for (int i = 0; i < 3; ++i) setLabelError(m_machCoord[i], QString()); break;
        case PI_FEED_OVERRIDE:    setLabelError(m_feedOverrideLabel, QString()); break;
        case PI_SPINDLE_OVERRIDE: setLabelError(m_spindleOverrideLabel, QString()); break;
        case PI_SPINDLE_SPEED:    setLabelError(m_spindleSpeedLabel, QString()); break;
        case PI_FEED_RATE_ORI:    setLabelError(m_feedRateLabel, QString()); break;
        case PI_RUN_TIME:   setLabelError(m_runTimeLabel, QString()); break;
        case PI_CUT_TIME:   setLabelError(m_cutTimeLabel, QString()); break;
        case PI_CYCLE_TIME: setLabelError(m_cycleTimeLabel, QString()); break;
        case PI_PRODUCTS:   setLabelError(m_productsLabel, QString()); break;
        }
    }

    // Advance
    m_pollIndex++;
    if (m_pollIndex >= PI_COUNT) {
        m_cycleActive = false;
        if (m_autoRefreshCheck->isChecked()) {
            int secs = m_intervalCombo->currentData().toInt();
            m_cycleDelayTimer->start(secs * 1000);
        }
    } else {
        m_interPollTimer->start();
    }
}

// ========== Polling Control ==========

void SyntecRealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    m_interPollTimer->stop();
    startCycle();
}

void SyntecRealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_interPollTimer->stop();
    m_cycleActive = false;
}

// ========== Clear Data ==========

void SyntecRealtimeWidget::clearData()
{
    resetLabel(m_statusLabel);
    resetLabel(m_modeLabel);
    resetLabel(m_progNameLabel);
    resetLabel(m_emgLabel);
    resetLabel(m_alarmLabel);
    resetLabel(m_productsLabel);
    resetLabel(m_feedRateLabel);
    resetLabel(m_feedOverrideLabel);
    resetLabel(m_spindleSpeedLabel);
    resetLabel(m_spindleOverrideLabel);
    resetLabel(m_runTimeLabel);
    resetLabel(m_cutTimeLabel);
    resetLabel(m_cycleTimeLabel);
    for (int i = 0; i < 3; ++i) {
        resetLabel(m_machCoord[i]);
        resetLabel(m_relCoord[i]);
    }
    m_hexDisplay->clear();
}

// ========== HEX Display ==========

void SyntecRealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
    m_hexDisplay->append(QString("[%1] %2").arg(isSend ? "TX" : "RX", hex.join(" ")));
}
```

- [ ] **Step 3: Commit**

```bash
git add src/protocol/syntec/syntecrealtimewidget.h src/protocol/syntec/syntecrealtimewidget.cpp
git commit -m "feat(syntec): add SyntecRealtimeWidget with 16-item polling and 3+1 layout"
```

---

### Task 6: MainWindow Integration

**Files:**
- Modify: `GSK988Tool.pro`
- Modify: `src/mainwindow.h`
- Modify: `src/mainwindow.cpp`

- [ ] **Step 1: Add Syntec files to GSK988Tool.pro**

Add after the KND block in both SOURCES and HEADERS sections:

In SOURCES (after `src/protocol/knd/kndparsewidget.cpp`):
```
    src/protocol/syntec/syntecframebuilder.cpp \
    src/protocol/syntec/syntecprotocol.cpp \
    src/protocol/syntec/syntecwidgetfactory.cpp \
    src/protocol/syntec/syntecrealtimewidget.cpp \
    src/protocol/syntec/synteccommandwidget.cpp \
    src/protocol/syntec/syntecparsewidget.cpp \
```

In HEADERS (after `src/protocol/knd/kndparsewidget.h`):
```
    src/protocol/syntec/syntecframebuilder.h \
    src/protocol/syntec/syntecprotocol.h \
    src/protocol/syntec/syntecwidgetfactory.h \
    src/protocol/syntec/syntecrealtimewidget.h \
    src/protocol/syntec/synteccommandwidget.h \
    src/protocol/syntec/syntecparsewidget.h \
```

- [ ] **Step 2: Add forward declarations to mainwindow.h**

After the KND forward declarations (`class KndProtocol;`), add:

```cpp
class SyntecRealtimeWidget;
class SyntecCommandWidget;
class SyntecProtocol;
```

- [ ] **Step 3: Add includes to mainwindow.cpp**

After the KND includes, add:

```cpp
#include "protocol/syntec/syntecprotocol.h"
#include "protocol/syntec/syntecwidgetfactory.h"
#include "protocol/syntec/syntecrealtimewidget.h"
#include "protocol/syntec/synteccommandwidget.h"
```

- [ ] **Step 4: Add combo box item in setupToolBar()**

After `m_protocolCombo->addItem("KND REST API");` add:

```cpp
    m_protocolCombo->addItem("Syntec 新代");
```

- [ ] **Step 5: Add case 6 in switchProtocol()**

In the `switch (m_protocolCombo->currentIndex())` block, after `case 5:` for KND, add:

```cpp
    case 6:
        m_protocol = new SyntecProtocol(this);
        m_widgetFactory = new SyntecWidgetFactory;
        break;
```

Also in switchProtocol(), in the "stop polling on current realtime widget" section, after the KND block, add:

```cpp
        auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRt) syntecRt->stopPolling();
```

- [ ] **Step 6: Add Syntec handshake in switchTransport() connected handler**

In the `connect(m_transport, &ITransport::connected, ...)` lambda, after the Siemens handshake block (`} else if (m_protocolCombo->currentIndex() == 3) { ... }`), add:

```cpp
        } else if (m_protocolCombo->currentIndex() == 6) {
            // Syntec: send handshake, polling starts after response
            m_needStartPolling = true;
            auto* syntec = qobject_cast<SyntecProtocol*>(m_protocol);
            QByteArray handshakeFrame = syntec->buildHandshake();
            m_transport->send(handshakeFrame);
            m_logTab->logFrame(handshakeFrame, true, "[握手] Syntec 新代");
        }
```

- [ ] **Step 7: Add Syntec disconnect handling in switchTransport()**

In the disconnected handler lambda, after the KND block, add:

```cpp
        auto* syntecRtDisc = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRtDisc) {
            syntecRtDisc->stopPolling();
            syntecRtDisc->clearData();
        }
```

- [ ] **Step 8: Add Syntec connections in setupProtocolConnections()**

After the Siemens command widget block, add:

```cpp
    // Syntec realtime widget
    auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
    if (syntecRt) {
        connect(syntecRt, &SyntecRealtimeWidget::pollRequest,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            auto* srt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
            if (srt) srt->appendHexDisplay(frame, true);

            m_timeoutTimer->start();
        });
    }

    // Syntec command widget
    auto* syntecCmd = qobject_cast<SyntecCommandWidget*>(m_commandTab);
    if (syntecCmd) {
        connect(syntecCmd, &SyntecCommandWidget::sendCommand,
                this, [this](quint8 cmdCode, const QByteArray& params) {
            if (!m_transport || !m_transport->isConnected()) return;

            QByteArray frame = m_protocol->buildRequest(cmdCode, params);
            m_waitingManualResponse = true;
            m_transport->send(frame);

            auto cdef = m_protocol->commandDef(cmdCode);
            QString desc = QString("[0x%1] %2")
                               .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                               .arg(cdef.name);
            m_logTab->logFrame(frame, true, desc);

            m_timeoutTimer->start();
        });
    }
```

- [ ] **Step 9: Add Syntec routing in onDataReceived()**

In `onDataReceived()`, after the Siemens realtime routing block, add:

```cpp
        // Route to Syntec realtime tab
        auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRt) {
            syntecRt->appendHexDisplay(frame, false);
            syntecRt->updateData(resp);
        }
```

Also in the `m_needStartPolling` block, add:

```cpp
            if (syntecRt) syntecRt->startPolling();
```

And in the `m_waitingManualResponse` block, add after the Siemens command widget routing:

```cpp
            auto* syntecCmdW = qobject_cast<SyntecCommandWidget*>(m_commandTab);
            if (syntecCmdW) {
                QString interp = m_protocol->interpretData(resp.cmdCode, resp.rawData);
                syntecCmdW->showResponse(resp, interp);
            }
```

- [ ] **Step 10: Add Syntec to onResponseTimeout()**

After the Siemens timeout block, add:

```cpp
    auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
    if (syntecRt) syntecRt->updateData(dummy);
```

- [ ] **Step 11: Add Syntec to onConnectClicked() disconnect section**

In the general disconnect section (after the KND block), add:

```cpp
        auto* syntecRt = qobject_cast<SyntecRealtimeWidget*>(m_realtimeTab);
        if (syntecRt) syntecRt->stopPolling();
```

- [ ] **Step 12: Commit**

```bash
git add GSK988Tool.pro src/mainwindow.h src/mainwindow.cpp
git commit -m "feat(syntec): integrate Syntec protocol into MainWindow with handshake and polling"
```

---

### Task 7: Build and Verify

- [ ] **Step 1: Create build batch file**

Create `build_syntec.bat`:

```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
cd /d D:\xieyi
qmake GSK988Tool.pro -spec win32-msvc
nmake
```

- [ ] **Step 2: Build**

Run:
```bash
cmd //c "D:\xieyi\build_syntec.bat"
```

Expected: Build completes with 0 errors. Warnings are acceptable.

- [ ] **Step 3: Fix any compilation errors**

Common issues:
- Missing `#include` directives — add them
- Case sensitivity in includes — fix to match actual filenames
- Forward declaration mismatches — check class names match exactly

- [ ] **Step 4: Final commit**

```bash
git add -A
git commit -m "fix(syntec): resolve build issues"
```

---

## Self-Review Checklist

1. **Spec coverage:**
   - FrameBuilder: all 19 commands with packet templates ✓
   - Protocol: IProtocol methods, tid management, mock responses ✓
   - RealtimeWidget: 16-item polling covering all spec items ✓
   - CommandWidget: query-only (no write commands per spec) ✓
   - ParseWidget: hex input with type selector ✓
   - MainWindow: combo item, case 6, handshake, connections ✓

2. **Placeholder scan:** No TBD, TODO, or "implement later" found.

3. **Type consistency:**
   - All signal/slot signatures use `(quint8 cmdCode, const QByteArray& params)` ✓
   - PollItem struct consistent with pollItems array ✓
   - SyntecFrameBuilder static methods match call sites ✓
   - ParsedResponse usage matches IProtocol interface ✓
