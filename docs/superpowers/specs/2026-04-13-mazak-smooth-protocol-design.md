# Mazak Smooth 协议集成设计

## 概述

将 Mazak Smooth CNC 协议集成到多协议调试工具中。Mazak 与现有协议不同，它通过闭源原生库 NTIFDLL.dll 通信，而非手动构建/解析 TCP 帧。采用 DLL 封装层 + 精简 IProtocol 适配方案，保持与现有架构的一致性。

## 架构

### 方案选择

**方案 A：DLL 封装层 + 精简 IProtocol**（已选定）

- MazakDLLWrapper 用 QLibrary 加载 NTIFDLL.dll
- MazakProtocol 实现 IProtocol 接口，但内部委托 DLL 调用
- 不走 Transport 层的帧收发流程，DLL 内部处理 TCP 通信
- 三件套 Widget 完全遵循现有模式

### 新增文件

```
src/protocol/mazak/
├── mazakdllwrapper.h/cpp      # QLibrary 加载 DLL，绑定 195 个导出函数
├── mazakprotocol.h/cpp        # 实现 IProtocol，委托 DLLWrapper
├── mazakframebuilder.h/cpp    # 数据结构定义 + 结构体/QByteArray 转换
├── mazakwidgetfactory.h/cpp   # 工厂
├── mazakrealtimewidget.h/cpp  # 实时监控
├── mazakcommandwidget.h/cpp   # 命令下发/参数写入
└── mazakparsewidget.h/cpp     # 数据解析
```

### 依赖关系

```
MainWindow
  ├── MazakProtocol (IProtocol)
  │     └── MazakDLLWrapper::instance() (单例, QLibrary)
  └── MazakWidgetFactory
        ├── MazakRealtimeWidget → MazakDLLWrapper
        ├── MazakCommandWidget → MazakDLLWrapper
        └── MazakParseWidget → MazakDLLWrapper
```

### 关键设计决策

1. **MazakDLLWrapper 单例** — 全局一个 DLL 实例，所有 Widget 共享连接句柄
2. **Transport 层旁路** — Mazak 不使用 ITransport，DLL 内部管理 TCP 连接
3. **延迟加载** — DLL 在用户点"连接"时加载，缺失不影响其他协议
4. **NTIFDLL.dll 分发** — 放在可执行文件同目录

---

## MazakDLLWrapper

### 职责

用 QLibrary 动态加载 NTIFDLL.dll，将 195 个 C 导出函数绑定为类型安全的 C++ 方法。

### 连接管理

```cpp
bool load(const QString& dllPath);       // 加载 DLL，解析所有函数指针
bool isLoaded() const;
void unload();
bool connect(ushort& handle, const QString& ip, int port, int timeout);  // MazConnect
bool disconnect(ushort handle);           // MazDisconnect
```

### 读取函数分组

DLL 导出了 195 个函数，按功能分组封装。每个方法返回 bool，失败时日志记录错误码。

| 分组 | 函数 | 用途 |
|------|------|------|
| 运行状态 | getRunningSts, getRunningInfo, getRunMode, getSkipSts | CNC 运行状态和模式 |
| 坐标 | getCurrentPos, getMachinePos, getRemain, getPositionInfo | 机械/绝对/剩余坐标 |
| 主轴 | getSpindleInfo, getSpindleLoad, getCurrentSpindleRev, getOrderSpindleRev, getSpindleOverRide | 转速、负载、倍率 |
| 进给 | getFeed, getFeedOverRide, getRapidOverRide | 进给速度和倍率 |
| 刀具 | getToolData, getCurrentTool, getToolLife, getToolOffsetInfo, getToolOffsetComp, getMagazineToolNum, getAllToolData | 刀具数据 |
| 报警 | getAlarm, getAlarmHistory, getNCAlarmInfo | 当前和历史报警 |
| 程序 | getMainProgram, getRunningProgram, getProgramFileList, getProgramDirInfo | 程序信息 |
| 参数/寄存器 | getRangedParam, getRangedCmnVar, getRangedRRegister, getRangedDRegister, getRangedZRRegister, getRangedAddWPC, getRangedWPEC, getRangedToolFile, getRangedToolOffsetComp, getRangedWorkCoordSys, getRangedSpAttachPara, getRangedTHPosition, getRangedJawData, getRangedPalMng | 各种参数区间读取 |
| PLC | getPLCDevSignal, getRangedPLCDevSignal | PLC 信号 |
| 时间 | getNcPowerOnTime, getRunningTime, getProTime, getPreparationTime, getAccumulatedTime | 运行时间统计 |
| 系统 | getMachineSerialNum, getMachineType, getMachineUnit, getAxisName, getAxisInfo, getPartsCount, getModal, getNCVersionInfo, getClientVer, getServerVer | 系统信息 |

### 写入函数分组

| 分组 | 函数 | 用途 |
|------|------|------|
| 参数 | setRangedParam, setRangedCmnVar, setRangedRRegister, setRangedZRRegister | 参数区间写入 |
| 刀具 | setToolData, setToolOffsetComp, setSMCTMOneToolData, setSMCTMOneToolOffset | 刀具数据写入 |
| 坐标 | setWorkCoordSys, setRangedWorkCoordSys, setRangedAddWPC | 坐标系设定 |
| 程序 | setMainProgram, sendProgram, receiveProgram, deleteProgram | 程序操作 |
| PLC | setRangedPLCDevSignal | PLC 信号写入 |
| 其他 | setBFSchedule, setPitchError, setAllCutCondition | 辅助功能 |
| 操作 | reset | NC 复位 |

### 函数签名推导

从 C# DllImport 声明反推 C 函数签名。示例：

```cpp
// C# 原始声明:
// [DllImport("NTIFDLL")] internal static extern int MazConnect(ref ushort param0, string param1, int param2, int param3);
// C++ 签名:
using ConnectFunc = int(*)(unsigned short*, const char*, int, int);

// C# 原始声明:
// [DllImport("NTIFDLL")] internal static extern int MazGetRunningInfo(ushort param0, ref MTC_ONE_RUNNING_INFO param1);
// C++ 签名:
using GetRunningInfoFunc = int(*)(unsigned short, MTC_ONE_RUNNING_INFO*);
```

---

## 数据结构

从 NTIFStruct.cs 移植，使用 `#pragma pack` 保证内存布局匹配。

### 核心结构体

```cpp
#pragma pack(push, 1)

struct MTC_ONE_RUNNING_INFO {
    short ncmode;        // 控制模式
    short ncsts;         // NC 状态
    short tno;           // 刀号
    short sp_override;   // 主轴倍率
    short ax_override;   // 进给倍率
    short rpid_override; // 快速移动倍率
    short almno;         // 报警号
    int   prtcnt;        // 加工件数
};

struct MTC_ONE_AXIS_INFO {
    char   axis_name[4]; // 轴名 "X", "Y", "Z" 等
    double pos;          // 绝对坐标
    double mc_pos;       // 机械坐标
    double load;         // 负载
    double feed;         // 进给速度
};

struct MTC_ONE_SPINDLE_INFO {
    double rot;          // 转速
    double load;         // 负载
    double temp;         // 温度
};

#pragma pack(pop)
```

### 辅助结构体

从 C# 源码移植的其他结构体包括：
- `NTIF_STRING` — 固定长度字符串（64 字节）
- `MTC_ONE_ALARM_INFO` — 报警信息
- `MTC_ONE_TOOL_DATA` — 刀具数据
- `MTC_ONE_TOOL_OFFSET` — 刀补数据
- `MTC_ONE_PROGRAM_INFO` — 程序信息
- `MTC_ONE_WORK_COORD` — 工件坐标系

所有字符串字段使用固定长度 char 数组 + MarshalAs 属性对应的布局。

### 错误码常量

```cpp
namespace MazakError {
    constexpr int OK        = 0;
    constexpr int SOCK      = -10;
    constexpr int HNDL      = -20;
    constexpr int CLIMAX    = -21;
    constexpr int SERVERMAX = -22;
    constexpr int VER       = -30;
    constexpr int BUSY      = -40;
    constexpr int RUNNING   = -50;
    constexpr int OVER      = -51;
    constexpr int NONE      = -52;
    constexpr int TYPE      = -53;
    constexpr int EDIT      = -54;
    constexpr int PROSIZE   = -55;
    constexpr int PRONUM    = -56;
    constexpr int ARG       = -60;
    constexpr int SYS       = -70;
    constexpr int FUNC      = -80;
    constexpr int TIMEOUT   = -90;
}
```

---

## MazakProtocol（IProtocol 适配）

### IProtocol 方法映射

由于 Mazak 不使用帧级通信，IProtocol 的部分方法做特殊适配：

| IProtocol 方法 | Mazak 实现 |
|---------------|-----------|
| `buildRequest()` | 返回标记 QByteArray，包含命令码和参数（不发送帧） |
| `parseResponse()` | 调用 DLL 读取函数，返回 ParsedResponse |
| `extractFrame()` | 直接返回 buffer 全部内容（DLL 不做帧处理） |
| `allCommands()` | 返回 DLL 支持的命令列表 |
| `commandDef()` | 返回命令元数据 |
| `interpretData()` | 格式化 DLL 返回数据为可读文本 |
| `mockResponse()` | 返回模拟数据（用于离线测试） |

### 命令编码

使用 quint8 编码 DLL 功能：

| 范围 | 功能 |
|------|------|
| 0x00 | 连接/断开 |
| 0x01-0x0F | 运行状态类 |
| 0x10-0x1F | 坐标类 |
| 0x20-0x2F | 主轴类 |
| 0x30-0x3F | 进给类 |
| 0x40-0x4F | 刀具类 |
| 0x50-0x5F | 报警类 |
| 0x60-0x6F | 程序类 |
| 0x70-0x7F | 参数/寄存器读取 |
| 0x80-0x8F | 参数/寄存器写入 |
| 0x90-0x9F | 系统信息类 |
| 0xA0-0xAF | 时间类 |
| 0xF0 | 复位 |

---

## MazakRealtimeWidget

### 布局

参照 Fanuc/Siemens 的 3+1 行布局：

**第 1 行 — 系统状态：**
- 运行状态（运行/停止/报警/待机）
- 运行模式（手动/自动/MDI/编辑）
- 当前程序名
- 加工件数

**第 2 行 — 主轴 + 进给：**
- 设定转速 / 实际转速
- 主轴负载 / 主轴倍率
- 进给速度 / 进给倍率
- 快速移动倍率

**第 3 行 — 坐标（动态轴数）：**
- 各轴：机械坐标、绝对坐标、剩余距离
- 轴名通过 getAxisName 动态获取

**底部信息栏：**
- 报警信息
- 开机时间 / 运行时间
- 当前刀号

### 轮询策略

- 间隔：500ms
- 每个轮询周期依次调用：
  1. `getRunningInfo` — 状态、模式、刀号、倍率、件数
  2. `getCurrentPos` — 各轴坐标
  3. `getSpindleInfo` — 主轴转速、负载
  4. `getFeed` — 进给速度
  5. `getAlarm` — 报警信息
- 超时处理：单次调用失败跳过，3 次连续失败标记断开

### 关键差异

与传统协议不同，Mazak RealtimeWidget **直接调用 DLL 函数**获取数据，不通过 MainWindow 的 send/receive 流程。轮询由 Widget 内部 QTimer 驱动。

---

## MazakCommandWidget

### 功能分类 Tab 页

**参数读写 Tab：**
- NC 参数 — `getRangedParam` / `setRangedParam`
- 宏变量 — `getRangedCmnVar` / `setRangedCmnVar`
- R 寄存器 — `getRangedRRegister` / `setRangedRRegister`
- D 寄存器 — `getRangedDRegister`

**刀具管理 Tab：**
- 刀具数据读写 — `getToolData` / `setToolData`
- 刀补读写 — `getToolOffsetComp` / `setToolOffsetComp`
- 刀具寿命 — `getToolLife`

**坐标设定 Tab：**
- 工件坐标系 — `getRangedWorkCoordSys` / `setRangedWorkCoordSys`
- 附加坐标系 — `getRangedAddWPC` / `setRangedAddWPC`

**程序操作 Tab：**
- 设定主程序 — `setMainProgram`
- 程序列表 — `getProgramFileList`

**操作 Tab：**
- NC 复位 — `reset`

### 交互模式

每个参数类型提供：
1. 输入起始编号 + 数量
2. 点"读取"调用 DLL Get 函数，显示结果到表格
3. 表格可编辑
4. 点"写入"调用 DLL Set 函数，将编辑后的值写回
5. 显示操作结果（成功/失败 + 错误码）

### 关键差异

与传统协议不同，Mazak CommandWidget **直接调用 DLL 函数**，不通过信号发送到 MainWindow。命令执行和结果显示完全在 Widget 内部完成。

---

## MazakParseWidget

简单数据解析工具：
- 下拉框选择数据类型（运行信息、坐标、主轴、刀具等）
- 十六进制输入区域
- 解析按钮
- 结构化结果显示

用于查看和验证 DLL 返回的原始内存数据。

---

## MainWindow 集成

### switchProtocol() 新增 case

```cpp
case 4:  // Mazak Smooth
    m_protocol = new MazakProtocol(this);
    m_widgetFactory = new MazakWidgetFactory;
    break;
```

### 协议下拉框

在 setupToolBar() 中添加：
```cpp
m_protocolCombo->addItem("Mazak Smooth");
```

### 连接流程差异

Mazak 的连接流程与其他协议显著不同：

1. **连接时**：在 onConnectClicked() 中，若当前协议是 Mazak：
   - 调用 `MazakDLLWrapper::load()` 加载 DLL
   - 从 Transport 配置获取 IP/端口/超时
   - 调用 `MazakDLLWrapper::connect()` 建立连接
   - **不使用** Transport 层的 connectToHost()
2. **断开时**：调用 `MazakDLLWrapper::disconnect()` + `unload()`

3. **数据流**：Widget 直接调用 DLL，不经过 onDataReceived()

### setupProtocolConnections()

Mazak 的 Widget 不需要连接到 MainWindow 的 pollRequest/sendCommand 信号槽，因为它们直接调用 DLL。setupProtocolConnections() 中对 Mazak 不做额外信号连接。

### Transport 层

Mazak 模式下 Transport 层不使用。Transport 下拉框可设为不可用或保持 TCP 选项仅用于配置 IP/端口/超时参数。

---

## 构建系统集成

### CMake 修改

在 CMakeLists.txt 中添加：
```cmake
src/protocol/mazak/mazakdllwrapper.h
src/protocol/mazak/mazakdllwrapper.cpp
src/protocol/mazak/mazakprotocol.h
src/protocol/mazak/mazakprotocol.cpp
src/protocol/mazak/mazakframebuilder.h
src/protocol/mazak/mazakframebuilder.cpp
src/protocol/mazak/mazakwidgetfactory.h
src/protocol/mazak/mazakwidgetfactory.cpp
src/protocol/mazak/mazakrealtimewidget.h
src/protocol/mazak/mazakrealtimewidget.cpp
src/protocol/mazak/mazakcommandwidget.h
src/protocol/mazak/mazakcommandwidget.cpp
src/protocol/mazak/mazakparsewidget.h
src/protocol/mazak/mazakparsewidget.cpp
```

### DLL 分发

- NTIFDLL.dll 复制到构建输出目录
- CMake 添加自定义命令自动复制

---

## 风险和待定项

1. **函数签名不确定** — 部分 DLL 函数的精确参数类型需要运行时验证。优先实现 C# 代码中已验证的函数，其余逐步补全。
2. **结构体内存对齐** — C# 的 StructLayout 和 C++ 的 pragma pack 可能不完全匹配，需要实测调整。
3. **线程安全** — DLL 的线程安全性未知，初期所有 DLL 调用在主线程执行，如有性能问题再引入工作线程。
4. **Mock 模式** — 由于 Mazak 不走 Transport 层，Mock 模式需要单独实现（在 DLLWrapper 层模拟返回数据）。
