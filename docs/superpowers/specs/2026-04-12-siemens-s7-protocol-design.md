# Siemens S7 协议设计

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 Siemens S7 (828D/840DSL/808D/802DSL) CNC 协议集成到多协议调试工具中，遵循 Fanuc FOCAS 的架构模式。

**Architecture:** 新建 `src/protocol/siemens/` 目录，包含 FrameBuilder（S7 命令字节数组 + 响应解析）、Protocol（IProtocol 实现，含 3 步握手）、RealtimeWidget（仪表盘）、CommandWidget（指令发送）、ParseWidget（帧解析）、WidgetFactory。MainWindow 增加 "Siemens S7" 协议选项。

**Tech Stack:** Qt 5.15.2 (C++14), MSVC 2019

**参考实现:** `C:\Users\QC\Desktop\828D 840dsl 808d 802dsl\SimensCNCTest\` (C# 源码)

---

## 1. S7 协议特征

### 1.1 帧格式

所有帧遵循 TPKT + COTP/S7Comm 格式：
```
字节0-3: TPKT 头 (03 00 XX XX，XX XX 为总长度)
字节4+: COTP/S7Comm 数据
```

帧长度由 TPKT 头的 bytes 2-3 决定（大端 16 位），`extractFrame()` 用此切分。

### 1.2 三步握手

S7 连接需要 3 步握手：

| 步骤 | 发送 | 说明 |
|------|------|------|
| 1 | COTP Connect Request (22字节) | COTP 连接请求 |
| 2 | S7 Communication Setup (25字节) | S7 通信参数协商 |
| 3 | S7 Setup (37字节) | S7 功能协商 |

每步发送后等待响应，收到响应后发送下一步。第 3 步完成后，握手结束。

### 1.3 命令格式

所有 S7 读取命令是预构建的字节数组（29-49字节），直接发送即可。特殊命令需要修改特定字节：
- **位置命令**：偏移 26 处修改轴标志 (0x01=X, 0x02=Y, 0x03=Z)
- **告警命令**：偏移 26 处修改告警索引
- **R 变量**：偏移 25-26 处修改变量地址
- **驱动参数**：偏移 22 处修改轴标志，偏移 24 处修改地址，偏移 26 处修改索引

### 1.4 响应解析

| 数据类型 | 解析方式 |
|----------|----------|
| String | UTF-8 从 offset 25，去 `\0` |
| Float | 4 字节 offset 25，**小端**（需 reverse） |
| Double | 8 字节 offset 25（当 frame[3]==33），否则 offset 0 |
| Int32 | 4 字节 offset 25，小端 |
| Int16 | 2 字节 offset 25，小端 |
| Mode | offset 24==0x02 时，offset 25 和 31 判断模式 |
| Status | offset 24==0x02 时，offset 25 和 31 判断状态 |

---

## 2. 内部命令编号

### 握手（内部，不显示在指令表）

| cmdCode | 说明 |
|---------|------|
| 0xFF | 握手第 1 步 |
| 0xFE | 握手第 2 步 |
| 0xFD | 握手第 3 步 |

### 系统信息

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x01 | CNC标识 | String | 机床指纹 |
| 0x02 | CNC型号 | String | 机床型号 |
| 0x03 | 版本信息 | String | NC 版本 |
| 0x04 | 制造日期 | String | 出厂日期 |

### 运行状态

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x05 | 操作模式 | Mode | JOG/MDA/AUTO/REFPOINT |
| 0x06 | 运行状态 | Status | RESET/STOP/START |

### 主轴

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x07 | 主轴设定速度 | Double | RPM |
| 0x08 | 主轴实际速度 | Double | RPM |
| 0x09 | 主轴倍率 | Double | % |

### 进给

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x0A | 进给设定速度 | Double | mm/min |
| 0x0B | 进给实际速度 | Double | mm/min |
| 0x0C | 进给倍率 | Double | % |

### 计数与时间

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x0D | 工件数 | Double | 实际加工数 |
| 0x0E | 设定工件数 | Double | 设定目标数 |
| 0x0F | 循环时间 | Double | 秒 |
| 0x10 | 剩余时间 | Double | 秒 |

### 程序与刀具

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x11 | 程序名 | String | 当前程序 |
| 0x12 | 程序内容 | String | 当前程序内容 |
| 0x13 | 刀具号 | Int16 | T 号 |
| 0x14 | 刀具半径D | Int16 | D 补偿号 |
| 0x15 | 刀具长度H | Int16 | H 补偿号 |
| 0x16 | 长度补偿X | Double | X 补偿值 |
| 0x17 | 长度补偿Z | Double | Z 补偿值 |
| 0x18 | 刀具磨损半径 | Double | 磨损值 |
| 0x19 | 刀沿位置 | Double | EDG |

### 坐标（每轴一个命令）

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x1A | 机械坐标-X | Double | posflag=0x01 |
| 0x1B | 机械坐标-Y | Double | posflag=0x02 |
| 0x1C | 机械坐标-Z | Double | posflag=0x03 |
| 0x1D | 工件坐标-X | Double | posflag=0x01 |
| 0x1E | 工件坐标-Y | Double | posflag=0x02 |
| 0x1F | 工件坐标-Z | Double | posflag=0x03 |
| 0x20 | 剩余坐标-X | Double | posflag=0x01 |
| 0x21 | 剩余坐标-Y | Double | posflag=0x02 |
| 0x22 | 剩余坐标-Z | Double | posflag=0x03 |
| 0x23 | 轴名称 | String | 读取轴名 |

### 驱动诊断

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x24 | 母线电压 | Float | |
| 0x25 | 驱动电流 | Float | |
| 0x26 | 电机功率-X | Float | |
| 0x27 | 驱动负载2 | Float | |
| 0x28 | 驱动负载3 | Float | |
| 0x29 | 驱动负载4 | Float | |
| 0x2A | 主轴负载1 | Float | |
| 0x2B | 主轴负载2 | Float | |
| 0x2C | 电机温度 | Float | |

### 告警

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x2D | 告警数量 | Int32 | |
| 0x2E | 告警信息 | Int32 | 需要告警索引参数 |

### 变量读写

| cmdCode | 名称 | 数据类型 | 说明 |
|---------|------|----------|------|
| 0x2F | 读R变量 | Double | 需要变量地址 |
| 0x30 | 写R变量 | — | 需要 addr + double value |
| 0x31 | R驱动器 | Float | 需要 addr+axis+index |

---

## 3. 文件结构

### 新建文件

| 文件 | 职责 |
|------|------|
| `src/protocol/siemens/siemensframebuilder.h` | S7 命令字节数组声明 + 响应解析函数 |
| `src/protocol/siemens/siemensframebuilder.cpp` | 实现：命令字节数组 + 解析辅助函数 |
| `src/protocol/siemens/siemensprotocol.h` | SiemensProtocol 类声明 |
| `src/protocol/siemens/siemensprotocol.cpp` | IProtocol 实现（含 3 步握手、buildRequest、parseResponse、mockResponse） |
| `src/protocol/siemens/siemensrealtimewidget.h` | 实时数据面板声明 |
| `src/protocol/siemens/siemensrealtimewidget.cpp` | 仪表盘 UI（网格布局，同 Fanuc 风格） |
| `src/protocol/siemens/siemenscommandwidget.h` | 指令发送面板声明 |
| `src/protocol/siemens/siemenscommandwidget.cpp` | 命令表格 + 参数输入 + 结果显示 |
| `src/protocol/siemens/siemensparsewidget.h` | 帧解析面板声明 |
| `src/protocol/siemens/siemensparsewidget.cpp` | HEX 输入 + 帧结构解析 |
| `src/protocol/siemens/siemenswidgetfactory.h` | Widget 工厂声明 |
| `src/protocol/siemens/siemenswidgetfactory.cpp` | 创建各 Widget 实例 |

### 修改文件

| 文件 | 修改内容 |
|------|----------|
| `src/mainwindow.h` | 前向声明 SiemensRealtimeWidget、SiemensCommandWidget |
| `src/mainwindow.cpp` | 协议组合框添加 "Siemens S7"、switchProtocol 新增 case 3、握手逻辑、信号连接、数据路由 |

---

## 4. SiemensFrameBuilder

静态工具类，不继承 QObject。

### 4.1 命令字节数组

每个命令定义为 `static const QByteArray` 常量。命名与 C# 源码一致：

```cpp
// 握手
static const QByteArray HANDSHAKE_1;  // COTP Connect
static const QByteArray HANDSHAKE_2;  // S7 Comm Setup
static const QByteArray HANDSHAKE_3;  // S7 Setup

// 命令（从 ETH_S7SimensCommands.cs 逐字翻译）
static const QByteArray CMD_CNC_ID;
static const QByteArray CMD_CNC_TYPE;
// ... 等
```

### 4.2 参数化命令构建

```cpp
// 位置命令：修改 base cmd 偏移 26 为 axisFlag
static QByteArray buildPositionCmd(const QByteArray& base, quint8 axisFlag);

// 告警命令：修改 base cmd 偏移 26 为 alarmIndex
static QByteArray buildAlarmCmd(int alarmIndex);

// R 变量：修改偏移 25-26 为变量地址
static QByteArray buildReadRCmd(int rAddr);

// 驱动参数：修改偏移 22(轴), 24(地址), 26(索引)
static QByteArray buildRDriverCmd(quint8 axisFlag, quint8 addrFlag, quint8 indexFlag);
```

### 4.3 响应解析辅助函数

```cpp
// 通用解析
static QString    parseString(const QByteArray& frame);     // offset 25, UTF-8
static float      parseFloat(const QByteArray& frame);      // offset 25, 小端 reverse
static double     parseDouble(const QByteArray& frame);     // offset 25 (if frame[3]==33)
static qint32     parseInt32(const QByteArray& frame);      // offset 25, 小端
static qint16     parseInt16(const QByteArray& frame);      // offset 25, 小端

// 特殊解析
static QString    parseMode(const QByteArray& frame);       // offset 24/25/31 → JOG/MDA/AUTO/REFPOINT
static QString    parseStatus(const QByteArray& frame);     // offset 24/25/31 → RESET/STOP/START
```

---

## 5. SiemensProtocol

继承 IProtocol (QObject)。

### 5.1 握手机制

```cpp
class SiemensProtocol : public IProtocol {
    // ...
    int m_handshakeStep;  // 0=未开始, 1=已发step1, 2=已发step2, 3=完成
    quint8 m_lastRequestCmdCode;

public:
    QByteArray buildHandshake();           // 返回 step 1 帧
    bool isHandshakeComplete() const;      // m_handshakeStep >= 3
    QByteArray advanceHandshake();         // 返回下一帧，推进 step
};
```

### 5.2 buildRequest

根据 cmdCode 返回对应命令字节数组。位置命令（0x1A-0x22）根据轴索引修改偏移 26。

### 5.3 extractFrame

从 buffer 中按 TPKT 头（bytes 2-3，大端 16 位）提取完整帧。

### 5.4 parseResponse

- 握手帧：识别 COTP/S7 响应，返回 `cmdCode=0xFF/0xFE/0xFD`，`isValid=true`
- 数据帧：通过 `m_lastRequestCmdCode` 确定 cmdCode，`rawData` = 从 offset 17 开始的数据（S7 数据区域）

### 5.5 mockResponse

为每个 cmdCode 生成合理的 mock 数据。握手响应返回标准确认帧。数据响应按类型生成：
- String: "828D" 等 mock 字符串
- Double: 1500.0 等 mock 数值
- Float: 380.5f 等 mock 数值
- Mode: AUTO
- Status: START

---

## 6. SiemensRealtimeWidget

### 6.1 轮询项目

固定轮询序列（22 项）：

| 索引 | cmdCode | 数据 |
|------|---------|------|
| 0 | 0x01 | CNC标识 |
| 1 | 0x05 | 操作模式 |
| 2 | 0x06 | 运行状态 |
| 3 | 0x07 | 主轴设定速度 |
| 4 | 0x08 | 主轴实际速度 |
| 5 | 0x09 | 主轴倍率 |
| 6 | 0x0A | 进给设定速度 |
| 7 | 0x0B | 进给实际速度 |
| 8 | 0x0C | 进给倍率 |
| 9 | 0x0D | 工件数 |
| 10 | 0x0F | 循环时间 |
| 11 | 0x11 | 程序名 |
| 12 | 0x13 | 刀具号 |
| 13-15 | 0x1A-0x1C | 机械坐标 X/Y/Z |
| 16-18 | 0x1D-0x1F | 工件坐标 X/Y/Z |
| 19-21 | 0x20-0x22 | 剩余坐标 X/Y/Z |

### 6.2 UI 布局

与 FanucRealtimeWidget 完全一致的网格布局：

```
[控制栏: 自动刷新 | 间隔 | 手动刷新]
[滚动区域:]
  [运行状态 | 主轴   | 进给    ]  ← Row 0, 3 列
  [系统信息 | 计数时间 | 程序刀具]  ← Row 1, 3 列
  [告警     | 坐标(2列span)     ]  ← Row 2
[HEX 显示]
```

### 6.3 updateData 逻辑

与 Fanuc 模式相同：
1. 根据 `m_pollIndex` 判断当前轮询项
2. 解析响应数据
3. 更新对应 QLabel
4. 推进到下一个轮询项
5. 所有项完成后，启动延迟定时器

---

## 7. SiemensCommandWidget

### 7.1 UI 结构

与 FanucCommandWidget 一致：
- 顶部：搜索框 + 分类过滤下拉
- 中部：命令表格（名称、功能码、分类、参数说明）
- 参数区：动态显示/隐藏参数输入框
- 底部：结果显示区

### 7.2 参数输入

| 命令类型 | 参数 |
|----------|------|
| 位置命令(0x1A-0x22) | 无（轴已内置在 cmdCode 中） |
| 告警(0x2E) | 告警索引 SpinBox |
| 读R变量(0x2F) | R 变量地址 SpinBox |
| 写R变量(0x30) | R 变量地址 + Double 值 |
| R驱动器(0x31) | 轴标志 + 地址 + 索引 |

### 7.3 显示/隐藏逻辑

根据选中命令的 cmdCode 决定参数区的可见性，与 FanucCommandWidget 的 currentCellChanged 逻辑相同。

---

## 8. SiemensParseWidget

简单的帧解析面板（同 FanucParseWidget）：
- HEX 输入框
- 解析按钮
- 结果显示区：显示 TPKT 头、COTP/S7 头、数据区域

---

## 9. MainWindow 集成

### 9.1 协议组合框

在 `setupToolBar()` 中添加：
```cpp
m_protocolCombo->addItem("Siemens S7");  // index 3
```

### 9.2 switchProtocol

在 switch 语句中添加 case 3：
```cpp
case 3:
    m_protocol = new SiemensProtocol(this);
    m_widgetFactory = new SiemensWidgetFactory;
    break;
```

### 9.3 连接握手

在 `switchTransport()` 的 connected lambda 中添加 Siemens 握手逻辑：
```cpp
else if (m_protocolCombo->currentIndex() == 3) {
    // Siemens S7: 3-step handshake
    m_needStartPolling = true;
    auto* siemens = qobject_cast<SiemensProtocol*>(m_protocol);
    QByteArray frame = siemens->buildHandshake();
    m_transport->send(frame);
    m_logTab->logFrame(frame, true, "[握手1/3] Siemens S7");
}
```

### 9.4 握手推进

在 `onDataReceived()` 中，解析帧后检查是否需要继续握手：
```cpp
// Handle Siemens multi-step handshake
auto* siemens = qobject_cast<SiemensProtocol*>(m_protocol);
if (siemens && !siemens->isHandshakeComplete()) {
    QByteArray nextFrame = siemens->advanceHandshake();
    m_transport->send(nextFrame);
    int step = siemens->handshakeStep();
    m_logTab->logFrame(nextFrame, true,
        QString("[握手%1/3] Siemens S7").arg(step));
    continue;  // 不路由到 realtime tab
}
```

### 9.5 数据路由

在 `onDataReceived()` 中添加 Siemens realtime widget 路由：
```cpp
auto* siemensRt = qobject_cast<SiemensRealtimeWidget*>(m_realtimeTab);
if (siemensRt) {
    siemensRt->appendHexDisplay(frame, false);
    siemensRt->updateData(resp);
}
```

### 9.6 信号连接

在 `setupProtocolConnections()` 中连接 SiemensRealtimeWidget 的 pollRequest 和 SiemensCommandWidget 的 sendCommand 信号，模式与其他协议完全一致。

---

## 10. MockServer 支持

SiemensProtocol::mockResponse() 需要为每个 cmdCode 返回合理的 mock 帧。

### 10.1 握手响应

返回标准 S7 确认帧（模仿真实设备行为）。

### 10.2 Mock 数据示例

| cmdCode | Mock 值 |
|---------|---------|
| 0x01 CNC_ID | "SIEMENS-828D" |
| 0x02 CNC_TYPE | "SINUMERIK 828D" |
| 0x05 CNC_MODE | AUTO (byte25=0x02, byte31=0x00) |
| 0x06 CNC_STATUS | START (byte25=0x01, byte31=0x03) |
| 0x07 主轴设定速度 | 2000.0 |
| 0x08 主轴实际速度 | 1500.0 |
| 0x09 主轴倍率 | 100.0 |
| 0x0A 进给设定速度 | 800.0 |
| 0x0B 进给实际速度 | 500.0 |
| 0x0C 进给倍率 | 120.0 |
| 0x0D 工件数 | 42.0 |
| 0x0F 循环时间 | 3600.0 |
| 0x11 程序名 | "O1234.MPF" |
| 0x13 刀具号 | 5 (Int16) |
| 0x1A 机械坐标-X | 100.5 |
| 0x1B 机械坐标-Y | 200.3 |
| 0x1C 机械坐标-Z | -50.0 |

### 10.3 Mock 响应帧构造

Mock 响应帧需要包含完整的 TPKT + S7Comm 头部（保持与真实响应相同结构）：
- TPKT 头 (4 字节)
- S7Comm 响应头 (21 字节)
- 数据 (从 offset 25 开始)

Mock 响应总长度 = 25 + 数据长度。

---

## 11. 验证方式

1. MSVC 编译通过：`powershell.exe -Command "cmd.exe /c 'D:\xieyi\do_build.bat'"`
2. 选择 "Siemens S7" 协议、"Mock模式"、点击"连接"
3. 验证 3 步握手日志显示 [握手1/3] [握手2/3] [握手3/3]
4. 连接后实时数据页自动刷新，显示 mock 数据
5. 操作模式显示 "AUTO"，运行状态显示 "START"
6. 坐标区域显示 X/Y/Z 三个值
7. 指令页选择命令发送，结果显示正确解析
8. HEX 显示收发帧数据
