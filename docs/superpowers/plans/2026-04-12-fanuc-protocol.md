# Fanuc FOCAS 协议插件实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 Fanuc FOCAS 以太网 CNC 数据采集协议集成为多协议调试工具的第三个协议插件。

**Architecture:** 遵循现有的 IProtocol + IProtocolWidgetFactory 架构，创建 `src/protocol/fanuc/` 目录下的完整插件，在 MainWindow 的协议下拉框中增加 "Fanuc FOCAS" 选项。

**Tech Stack:** Qt 5.15.2 (C++14), MSVC 2019

---

### Task 1: FanucFrameBuilder — 帧编码/解码工具

**Files:**
- Create: `src/protocol/fanuc/fanucframebuilder.h`
- Create: `src/protocol/fanuc/fanucframebuilder.cpp`

- [ ] **Step 1: 创建 fanucframebuilder.h**

```cpp
#ifndef FANUCFRAMEBUILDER_H
#define FANUCFRAMEBUILDER_H

#include <QByteArray>

class FanucFrameBuilder
{
public:
    // 协议头
    static constexpr quint8 HEADER_BYTE = 0xA0;

    // 请求功能码
    static const QByteArray FUNC_READ;       // 0x00 0x01 0x21 0x01
    // 响应功能码
    static const QByteArray FUNC_RESPONSE;   // 0x00 0x01 0x21 0x02

    // ===== 帧构建 =====

    // 构建握手帧
    static QByteArray buildHandshake();

    // 构建单数据块请求帧
    static QByteArray buildSingleBlockRequest(quint16 ncFlag, quint16 blockFuncCode,
                                               const QByteArray& blockParams);
    // 构建多数据块请求帧（用于读时间等需要两个数据块的场景）
    static QByteArray buildMultiBlockRequest(const QVector<QByteArray>& blocks);

    // 构建响应帧（Mock用）
    static QByteArray buildResponse(const QByteArray& dataPayload);

    // ===== 数据块构建 =====

    // 构建标准28字节数据块
    static QByteArray buildBlock(quint16 ncFlag, quint16 blockFuncCode,
                                  const QByteArray& params20);

    // 构建读PMC数据块
    static QByteArray buildReadPMCBlock(char addrType, int startAddr, int endAddr,
                                         int pmcTypeNum, int dataType);

    // 构建写PMC数据块 (byte)
    static QByteArray buildWritePMCBlockByte(char addrType, int startAddr, int endAddr,
                                              int pmcTypeNum, quint8 value);
    // 构建写PMC数据块 (word)
    static QByteArray buildWritePMCBlockWord(char addrType, int startAddr, int endAddr,
                                              int pmcTypeNum, qint16 value);
    // 构建写PMC数据块 (long)
    static QByteArray buildWritePMCBlockLong(char addrType, int startAddr, int endAddr,
                                              int pmcTypeNum, qint32 value);
    // 构建写PMC数据块 (float)
    static QByteArray buildWritePMCBlockFloat(char addrType, int startAddr, int endAddr,
                                               int pmcTypeNum, float value);

    // 构建读宏变量数据块
    static QByteArray buildReadMacroBlock(int addr);

    // 构建写宏变量数据块
    static QByteArray buildWriteMacroBlock(int addr, int numerator, quint8 precision);

    // 构建读参数数据块
    static QByteArray buildReadParamBlock(int paramNo);

    // 构建获取坐标数据块
    static QByteArray buildPositionBlock(quint8 posType);

    // ===== 帧解析 =====

    // 从buffer中提取一个完整帧
    static QByteArray extractFrame(QByteArray& buffer);

    // 校验帧头
    static bool validateHeader(const QByteArray& frame);

    // 获取帧中的数据长度字段
    static quint16 getDataLength(const QByteArray& frame);

    // ===== 数据编码 =====

    // int32 大端编码
    static QByteArray encodeInt32(qint32 value);
    // int32 大端解码
    static qint32 decodeInt32(const QByteArray& data, int offset = 0);
    // short 大端编码
    static QByteArray encodeInt16(qint16 value);
    // short 大端解码
    static qint16 decodeInt16(const QByteArray& data, int offset = 0);
    // calcValue: 分子(int32) / 分母底(short)^指数(short)
    static double calcValue(const QByteArray& data, int offset = 0);

    // PMC地址类型字母转编号
    static quint8 pmcAddrTypeToNum(char addrType);
    // PMC地址类型编号转字母
    static char pmcNumToAddrType(quint8 num);
};

#endif // FANUCFRAMEBUILDER_H
```

- [ ] **Step 2: 创建 fanucframebuilder.cpp**

实现所有声明的方法。关键实现要点：

- `buildSingleBlockRequest`: 拼装 `A0×4` + `FUNC_READ` + 数据长度(2B) + 块个数(2B) + 数据块(28B)
- `buildMultiBlockRequest`: 拼装 `A0×4` + `FUNC_READ` + 总数据长度 + 块个数 + 多个数据块
- `buildBlock`: 28字节 = 块长度(00 1C) + NC/PMC(2B) + 预留(00 01) + 块功能码(2B) + 参数区(20B)
- `extractFrame`: 读取 offset 8-9 的数据长度，总帧 = 10 + 数据长度
- `calcValue`: `decodeInt32(data, offset) / pow(decodeInt16(data, offset+4), decodeInt16(data, offset+6))`
- `encodeInt32`: 大端序 `(value>>24)&0xFF, (value>>16)&0xFF, (value>>8)&0xFF, value&0xFF`
- `encodeInt16`: 大端序 `(value>>8)&0xFF, value&0xFF`
- `pmcAddrTypeToNum`: G=0, F=1, Y=2, X=3, A=4, R=5, T=6, K=7, C=8, D=9

```cpp
#include "fanucframebuilder.h"
#include <cmath>

const QByteArray FanucFrameBuilder::FUNC_READ  = QByteArray("\x00\x01\x21\x01", 4);
const QByteArray FanucFrameBuilder::FUNC_RESPONSE = QByteArray("\x00\x01\x21\x02", 4);

QByteArray FanucFrameBuilder::buildHandshake()
{
    QByteArray frame;
    frame.append(4, HEADER_BYTE);                        // A0 A0 A0 A0
    frame.append(FUNC_READ);                             // 00 01 21 01
    frame.append(encodeInt16(0x0002));                   // 数据长度=2
    frame.append(encodeInt16(0x0002));                   // 数据内容=0x0002
    return frame;
}

QByteArray FanucFrameBuilder::buildSingleBlockRequest(
    quint16 ncFlag, quint16 blockFuncCode, const QByteArray& blockParams)
{
    QByteArray block = buildBlock(ncFlag, blockFuncCode, blockParams);
    QByteArray frame;
    frame.append(4, HEADER_BYTE);
    frame.append(FUNC_READ);
    // 数据长度 = 块个数(2B) + 块数据(28B) = 30
    frame.append(encodeInt16(2 + block.size()));
    frame.append(encodeInt16(1));  // 1个数据块
    frame.append(block);
    return frame;
}

QByteArray FanucFrameBuilder::buildMultiBlockRequest(const QVector<QByteArray>& blocks)
{
    int totalBlockData = 0;
    for (const auto& b : blocks) totalBlockData += b.size();
    int dataLen = 2 + totalBlockData; // 块个数(2B) + 所有块

    QByteArray frame;
    frame.append(4, HEADER_BYTE);
    frame.append(FUNC_READ);
    frame.append(encodeInt16(static_cast<qint16>(dataLen)));
    frame.append(encodeInt16(static_cast<qint16>(blocks.size())));
    for (const auto& b : blocks) frame.append(b);
    return frame;
}

QByteArray FanucFrameBuilder::buildResponse(const QByteArray& dataPayload)
{
    QByteArray frame;
    frame.append(4, HEADER_BYTE);
    frame.append(FUNC_RESPONSE);
    frame.append(encodeInt16(static_cast<qint16>(dataPayload.size())));
    frame.append(dataPayload);
    return frame;
}

QByteArray FanucFrameBuilder::buildBlock(
    quint16 ncFlag, quint16 blockFuncCode, const QByteArray& params20)
{
    QByteArray block;
    block.append(encodeInt16(0x001C));                   // 块长度=28
    block.append(encodeInt16(ncFlag));                   // NC/PMC
    block.append(encodeInt16(0x0001));                   // 预留
    block.append(encodeInt16(blockFuncCode));             // 块功能码
    // 参数区补齐到20字节
    QByteArray padded = params20;
    padded.resize(20, '\0');
    block.append(padded);
    return block;
}

QByteArray FanucFrameBuilder::buildReadPMCBlock(
    char addrType, int startAddr, int endAddr, int pmcTypeNum, int dataType)
{
    QByteArray params;
    params.append(encodeInt16(0x0002));  // NC=1, PMC=2
    params.append(encodeInt16(0x0001));  // 预留
    params.append(encodeInt16(0x8001));  // 读PMC功能码
    params.append(encodeInt32(startAddr));
    params.append(encodeInt32(endAddr));
    params.append(encodeInt32(pmcTypeNum));
    params.append(encodeInt32(dataType));
    params.append(QByteArray(4, '\0'));  // 对齐到20字节
    // 总共: 2+2+2+4+4+4+4+4 = 26... 需要调整
    // 标准PMC读块: 28字节 = 块长度(2)+NC/PMC(2)+预留(2)+功能码(2)+参数(20)
    // 参数20字节: startAddr(4)+endAddr(4)+typeNum(4)+dataType(4)+padding(4)
    QByteArray block;
    block.append(encodeInt16(0x001C));
    block.append(encodeInt16(0x0002));  // PMC
    block.append(encodeInt16(0x0001));
    block.append(encodeInt16(0x8001));
    block.append(encodeInt32(startAddr));
    block.append(encodeInt32(endAddr));
    block.append(encodeInt32(pmcTypeNum));
    params = QByteArray();
    params.append(encodeInt32(dataType));
    params.append(QByteArray(4, '\0'));
    block.append(params);
    return block;
}

// ... 其余 PMC 写数据块、宏变量、参数、坐标的构建方法类似
// 每个都是构造一个28字节的block，填入对应的参数

QByteArray FanucFrameBuilder::extractFrame(QByteArray& buffer)
{
    if (buffer.size() < 10) return QByteArray();
    if (static_cast<quint8>(buffer[0]) != HEADER_BYTE) {
        buffer.clear();
        return QByteArray();
    }
    quint16 dataLen = decodeInt16(buffer, 8);
    int totalFrame = 10 + dataLen;
    if (buffer.size() < totalFrame) return QByteArray();
    QByteArray frame = buffer.left(totalFrame);
    buffer.remove(0, totalFrame);
    return frame;
}

bool FanucFrameBuilder::validateHeader(const QByteArray& frame)
{
    if (frame.size() < 4) return false;
    return static_cast<quint8>(frame[0]) == HEADER_BYTE &&
           static_cast<quint8>(frame[1]) == HEADER_BYTE &&
           static_cast<quint8>(frame[2]) == HEADER_BYTE &&
           static_cast<quint8>(frame[3]) == HEADER_BYTE;
}

quint16 FanucFrameBuilder::getDataLength(const QByteArray& frame)
{
    if (frame.size() < 10) return 0;
    return static_cast<quint16>(decodeInt16(frame, 8));
}

QByteArray FanucFrameBuilder::encodeInt32(qint32 value)
{
    QByteArray ba(4, '\0');
    ba[0] = static_cast<char>((value >> 24) & 0xFF);
    ba[1] = static_cast<char>((value >> 16) & 0xFF);
    ba[2] = static_cast<char>((value >> 8) & 0xFF);
    ba[3] = static_cast<char>(value & 0xFF);
    return ba;
}

qint32 FanucFrameBuilder::decodeInt32(const QByteArray& data, int offset)
{
    if (data.size() < offset + 4) return 0;
    return (static_cast<quint8>(data[offset]) << 24) |
           (static_cast<quint8>(data[offset+1]) << 16) |
           (static_cast<quint8>(data[offset+2]) << 8) |
            static_cast<quint8>(data[offset+3]);
}

QByteArray FanucFrameBuilder::encodeInt16(qint16 value)
{
    QByteArray ba(2, '\0');
    ba[0] = static_cast<char>((value >> 8) & 0xFF);
    ba[1] = static_cast<char>(value & 0xFF);
    return ba;
}

qint16 FanucFrameBuilder::decodeInt16(const QByteArray& data, int offset)
{
    if (data.size() < offset + 2) return 0;
    return static_cast<qint16>(
        (static_cast<quint8>(data[offset]) << 8) |
         static_cast<quint8>(data[offset+1]));
}

double FanucFrameBuilder::calcValue(const QByteArray& data, int offset)
{
    qint32 numerator = decodeInt32(data, offset);
    qint16 denomBase = decodeInt16(data, offset + 4);
    qint16 denomExp = decodeInt16(data, offset + 6);
    double denominator = std::pow(static_cast<double>(denomBase),
                                   static_cast<double>(denomExp));
    if (denominator == 0.0) return 0.0;
    return static_cast<double>(numerator) / denominator;
}

quint8 FanucFrameBuilder::pmcAddrTypeToNum(char addrType)
{
    switch (addrType) {
    case 'G': return 0;  case 'F': return 1;
    case 'Y': return 2;  case 'X': return 3;
    case 'A': return 4;  case 'R': return 5;
    case 'T': return 6;  case 'K': return 7;
    case 'C': return 8;  case 'D': return 9;
    default:  return 0;
    }
}

char FanucFrameBuilder::pmcNumToAddrType(quint8 num)
{
    static const char types[] = "GFYXARTKCD";
    return (num < 10) ? types[num] : 'G';
}
```

- [ ] **Step 3: 编译验证**

Run: `powershell.exe -Command "cmd.exe /c 'D:\xieyi\do_build.bat'"` (先在 .pro 中加入文件再编译)
Expected: 编译通过，无错误

- [ ] **Step 4: Commit**

```bash
git add src/protocol/fanuc/fanucframebuilder.h src/protocol/fanuc/fanucframebuilder.cpp
git commit -m "feat(fanuc): add FanucFrameBuilder for FOCAS frame encoding/decoding"
```

---

### Task 2: FanucProtocol — IProtocol 实现

**Files:**
- Create: `src/protocol/fanuc/fanucprotocol.h`
- Create: `src/protocol/fanuc/fanucprotocol.cpp`

- [ ] **Step 1: 创建 fanucprotocol.h**

```cpp
#ifndef FANUCPROTOCOL_H
#define FANUCPROTOCOL_H

#include "protocol/iprotocol.h"
#include <QVector>

struct FanucCommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

class FanucProtocol : public IProtocol {
    Q_OBJECT
public:
    explicit FanucProtocol(QObject* parent = nullptr);

    QString name() const override { return "Fanuc FOCAS"; }
    QString version() const override { return "1.0"; }

    // 握手帧单独构建（不走 buildRequest 的 cmdCode 体系）
    QByteArray buildHandshake() const;

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {}) override;
    ParsedResponse parseResponse(const QByteArray& frame) override;
    QByteArray extractFrame(QByteArray& buffer) override;

    QVector<ProtocolCommand> allCommands() const override;
    ProtocolCommand commandDef(quint8 cmdCode) const override;

    QString interpretData(quint8 cmdCode, const QByteArray& data) const override;
    QByteArray mockResponse(quint8 cmdCode, const QByteArray& requestData) const override;

    // Raw access
    static QVector<FanucCommandDef> allCommandsRaw();
    static FanucCommandDef commandDefRaw(quint8 cmdCode);
    static QString interpretDataRaw(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);

private:
    bool m_handshakeDone;
};

#endif // FANUCPROTOCOL_H
```

- [ ] **Step 2: 创建 fanucprotocol.cpp**

实现所有接口。关键要点：

**命令定义** (`buildCommandTable`):
```
cmdCode | 名称           | 类别     | 块功能码
0x01    | 系统信息        | 系统信息  | 0x18
0x02    | 运行信息        | 运行状态  | 0x19
0x03    | 主轴速度        | 主轴      | 0x25
0x04    | 主轴负载        | 主轴      | 0x40
0x05    | 主轴倍率        | 主轴      | PMC读G30
0x06    | 主轴速度设定     | 主轴      | PMC读F22-25
0x07    | 进给速度        | 进给      | 0x24
0x08    | 进给倍率        | 进给      | PMC读G12
0x09    | 进给速度设定     | 进给      | 宏变量4109
0x0A    | 工件数          | 计数      | 宏变量3901
0x0B    | 运行时间        | 时间      | 参数6751+6752
0x0C    | 加工时间        | 时间      | 参数6753+6754
0x0D    | 循环时间        | 时间      | 参数6757+6758
0x0E    | 上电时间        | 时间      | 参数6750
0x0F    | 程序号          | 程序      | 0x1C
0x10    | 刀具号          | 程序      | 宏变量4120
0x11    | 告警信息        | 告警      | 0x23
0x12    | 绝对坐标        | 坐标      | 0x26 type=0
0x13    | 机械坐标        | 坐标      | 0x26 type=1
0x14    | 相对坐标        | 坐标      | 0x26 type=2
0x15    | 剩余距离坐标    | 坐标      | 0x26 type=3
0x16    | 读宏变量        | 宏变量    | 0x15
0x17    | 写宏变量        | 宏变量    | 0x16
0x18    | 读PMC           | PMC       | 0x8001
0x19    | 写PMC           | PMC       | 0x8002
0x1A    | 读参数          | 参数      | 0x8D
```

**buildRequest**: 根据 cmdCode 调用 FanucFrameBuilder 的对应方法构建帧
**parseResponse**: 校验帧头和功能码，提取 rawData（帧偏移10开始）
**extractFrame**: 委托给 FanucFrameBuilder::extractFrame
**mockResponse**: 为每个 cmdCode 生成模拟响应帧

Mock 数据示例:
- 系统信息: NC型号="31", 设备类型="0B"
- 运行信息: 模式=1(MEM), 状态=1(STOP), 急停=0, 告警=0
- 主轴速度: 1500 RPM
- 进给速度: 500 mm/min
- 坐标: X=100.50, Y=200.30, Z=-50.00
- 告警: 空列表

- [ ] **Step 3: 编译验证**

- [ ] **Step 4: Commit**

```bash
git add src/protocol/fanuc/fanucprotocol.h src/protocol/fanuc/fanucprotocol.cpp
git commit -m "feat(fanuc): add FanucProtocol with command definitions and mock responses"
```

---

### Task 3: FanucWidgetFactory + .pro 文件更新

**Files:**
- Create: `src/protocol/fanuc/fanucwidgetfactory.h`
- Create: `src/protocol/fanuc/fanucwidgetfactory.cpp`
- Modify: `GSK988Tool.pro` — 添加所有 Fanuc 源文件
- Modify: `src/protocol/fanuc/fanuccommandwidget.h` (placeholder)
- Modify: `src/protocol/fanuc/fanuccommandwidget.cpp` (placeholder)
- Modify: `src/protocol/fanuc/fanucparsewidget.h` (placeholder)
- Modify: `src/protocol/fanuc/fanucparsewidget.cpp` (placeholder)
- Modify: `src/protocol/fanuc/fanucrealtimewidget.h` (placeholder)
- Modify: `src/protocol/fanuc/fanucrealtimewidget.cpp` (placeholder)

- [ ] **Step 1: 创建 WidgetFactory 和 placeholder widgets**

`fanucwidgetfactory.h`:
```cpp
#ifndef FANUCWIDGETFACTORY_H
#define FANUCWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class FanucWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // FANUCWIDGETFACTORY_H
```

`fanucwidgetfactory.cpp`:
```cpp
#include "fanucwidgetfactory.h"
#include "fanucrealtimewidget.h"
#include "fanuccommandwidget.h"
#include "fanucparsewidget.h"

QWidget* FanucWidgetFactory::createRealtimeWidget()
{
    return new FanucRealtimeWidget;
}

QWidget* FanucWidgetFactory::createCommandWidget()
{
    return new FanucCommandWidget;
}

QWidget* FanucWidgetFactory::createParseWidget()
{
    return new FanucParseWidget;
}
```

三个 placeholder widget 各创建最小实现（仅含构造函数和 setupUI 空壳）：
- `FanucRealtimeWidget` — 参考 Gsk988RealtimeWidget 接口（startPolling/stopPolling/clearData/appendHexDisplay/pollRequest 信号）
- `FanucCommandWidget` — 含 sendCommand 信号和 showResponse 方法
- `FanucParseWidget` — 含 hex 输入和解析按钮

- [ ] **Step 2: 更新 GSK988Tool.pro**

在 SOURCES 和 HEADERS 中添加所有 fanuc 文件。

- [ ] **Step 3: 编译验证**

- [ ] **Step 4: Commit**

```bash
git add src/protocol/fanuc/ GSK988Tool.pro
git commit -m "feat(fanuc): add FanucWidgetFactory and placeholder widgets"
```

---

### Task 4: FanucRealtimeWidget — 全量轮询实时数据页面

**Files:**
- Modify: `src/protocol/fanuc/fanucrealtimewidget.h`
- Modify: `src/protocol/fanuc/fanucrealtimewidget.cpp`

- [ ] **Step 1: 完善 fanucrealtimewidget.h**

参考 Gsk988RealtimeWidget 的轮询模式。声明：
- 轮询项索引枚举（PI_SYSINFO 到 PI_POS_RES，共21项）
- PollItem 结构体（cmdCode + params）
- static const PollItem 数组
- QLabel 成员变量（按数据分组：系统信息、运行状态、主轴组、进给组、计数/时间、程序/刀具、告警、坐标XYZ×4组）
- QTimer* m_cycleDelayTimer
- bool m_cycleActive; int m_pollIndex; int m_pollCount;

- [ ] **Step 2: 完善 fanucrealtimewidget.cpp — pollItems 数组**

定义21个轮询项，每项包含 cmdCode 和需要的 params（例如坐标类型、PMC地址等）。

- [ ] **Step 3: 完善 fanucrealtimewidget.cpp — setupUI()**

布局：
- 上方：自动刷新 checkbox + 间隔 combo + 手动刷新按钮
- 中间（QScrollArea 内 QGridLayout）：按类别分组显示数据项
  - 系统信息组: NC型号、设备类型
  - 运行状态组: 操作模式、运行状态、急停、告警状态
  - 主轴组: 速度、负载、倍率、速度设定值
  - 进给组: 速度、倍率、速度设定值
  - 计数/时间组: 工件数、运行时间、加工时间、循环时间、上电时间
  - 程序/刀具组: 程序号、刀具号
  - 告警组: 告警列表（QTextEdit）
  - 坐标组: 绝对/机械/相对/剩余 各XYZ（4行×3列）
- 下方: Hex 收发显示

- [ ] **Step 4: 完善 fanucrealtimewidget.cpp — startCycle/stopPolling/updateData**

- `startCycle()`: 从 pollItems[m_pollIndex] 取出 cmdCode 和 params，emit pollRequest
- `updateData()`: 根据 m_pollIndex 判断当前是哪个数据项，用 FanucFrameBuilder::calcValue 等方法解析响应数据，更新对应 QLabel
- 轮询完成后启动 m_cycleDelayTimer 等待下一轮

- [ ] **Step 5: 编译验证**

- [ ] **Step 6: Commit**

```bash
git add src/protocol/fanuc/fanucrealtimewidget.h src/protocol/fanuc/fanucrealtimewidget.cpp
git commit -m "feat(fanuc): add FanucRealtimeWidget with full polling cycle"
```

---

### Task 5: FanucCommandWidget — 手动发送指令页面

**Files:**
- Modify: `src/protocol/fanuc/fanuccommandwidget.h`
- Modify: `src/protocol/fanuc/fanuccommandwidget.cpp`

- [ ] **Step 1: 完善 fanuccommandwidget.h**

参考 ModbusCommandWidget 结构：
- 左侧：命令表格（类型/功能码/名称）+ 搜索/类别过滤
- 右侧：参数面板 + 发送按钮 + 结果显示
- 信号: sendCommand(quint8 cmdCode, const QByteArray& params)
- 方法: showResponse(const ParsedResponse&, const QString& interpretation)

- [ ] **Step 2: 完善 fanuccommandwidget.cpp — setupUI**

左侧表格按类别颜色标记显示命令列表。右侧参数面板根据选中命令动态显示/隐藏：
- 读宏变量: 地址输入
- 写宏变量: 地址 + 值输入
- 读PMC: 地址类型(Combo G/F/Y/X/...) + 地址 + 数据类型(byte/word/long/float)
- 写PMC: 地址类型 + 地址 + 值 + 数据类型
- 读参数: 参数号
- 坐标: 坐标类型 Combo（绝对/机械/相对/剩余）
- 其他命令: 无额外参数

- [ ] **Step 3: 完善 fanuccommandwidget.cpp — 发送逻辑**

发送按钮根据选中命令的 cmdCode 构建 params 并 emit sendCommand。

- [ ] **Step 4: 编译验证**

- [ ] **Step 5: Commit**

```bash
git add src/protocol/fanuc/fanuccommandwidget.h src/protocol/fanuc/fanuccommandwidget.cpp
git commit -m "feat(fanuc): add FanucCommandWidget with manual command sending"
```

---

### Task 6: FanucParseWidget — 手动帧解析页面

**Files:**
- Modify: `src/protocol/fanuc/fanucparsewidget.h`
- Modify: `src/protocol/fanuc/fanucparsewidget.cpp`

- [ ] **Step 1: 完善 fanucparsewidget**

参考 ModbusParseWidget 模式：
- Hex 输入框 + 解析按钮
- 结果显示：解析帧结构（协议头、功能码、数据长度、数据块个数、每个数据块的详细内容）
- 用 FanucProtocol 静态方法解析

- [ ] **Step 2: 编译验证**

- [ ] **Step 3: Commit**

```bash
git add src/protocol/fanuc/fanucparsewidget.h src/protocol/fanuc/fanucparsewidget.cpp
git commit -m "feat(fanuc): add FanucParseWidget for manual frame analysis"
```

---

### Task 7: MainWindow 集成

**Files:**
- Modify: `src/mainwindow.cpp`
- Modify: `src/mainwindow.h`

- [ ] **Step 1: 添加 includes**

在 mainwindow.cpp 顶部添加：
```cpp
#include "protocol/fanuc/fanucprotocol.h"
#include "protocol/fanuc/fanucwidgetfactory.h"
#include "protocol/fanuc/fanucrealtimewidget.h"
```

- [ ] **Step 2: 协议下拉框添加选项**

在 setupToolBar() 中 m_protocolCombo 添加 "Fanuc FOCAS"。

- [ ] **Step 3: switchProtocol 添加 case 2**

```cpp
case 2:
    m_protocol = new FanucProtocol(this);
    m_widgetFactory = new FanucWidgetFactory;
    break;
```

在 setupProtocolConnections() 中添加 FanucRealtimeWidget 的 pollRequest 连接（参考 Modbus 部分）。

- [ ] **Step 4: 连接握手逻辑**

在 transport connected 信号的回调中，当 `m_protocolCombo->currentIndex() == 2` 时：
1. 发送握手帧
2. 设置 `m_needStartPolling = true`
3. 收到握手响应后启动轮询

- [ ] **Step 5: onDataReceived 添加 Fanuc 路由**

添加 FanucRealtimeWidget 和 FanucCommandWidget 的 qobject_cast 和数据分发。

- [ ] **Step 6: 编译验证**

- [ ] **Step 7: 运行测试**

手动启动程序，切换到 Fanuc FOCAS 协议，用 Mock 模式连接，验证：
1. 默认显示实时数据页面
2. 握手自动发送
3. 全量轮询数据显示正确
4. 切换回其他协议正常

- [ ] **Step 8: Commit**

```bash
git add src/mainwindow.cpp src/mainwindow.h
git commit -m "feat(fanuc): integrate Fanuc FOCAS protocol into MainWindow"
```
