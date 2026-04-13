# 新代 (Syntec) CNC 协议集成设计

## 概述

将新代 (Syntec) CNC 的专有二进制协议集成到多协议调试工具中。新代通过 TCP Socket 通信，使用固定格式的二进制报文（小端序），通信前需握手。采用 Transport 层 + IProtocol 方案，与 GSK988/Fanuc/Siemens 一致。

## 架构

### 方案选择

**方案 A：Transport 层 + IProtocol**（已选定）

- 走标准 TCP Transport 层
- SyntecProtocol 实现 IProtocol，每个命令的请求包硬编码
- extractFrame 根据响应头部长度字段提取完整帧
- 三件套 Widget 通过 MainWindow 的 pollRequest/sendCommand 信号通信

### 新增文件

```
src/protocol/syntec/
├── syntecframebuilder.h/cpp      # 报文构建/解析，状态/模式映射
├── syntecprotocol.h/cpp          # IProtocol 实现，命令定义
├── syntecwidgetfactory.h/cpp     # 工厂
├── syntecrealtimewidget.h/cpp    # 实时监控
├── synteccommandwidget.h/cpp     # 命令下发
└── syntecparsewidget.h/cpp       # 数据解析
```

### 依赖关系

```
MainWindow
  ├── SyntecProtocol (IProtocol)
  │     └── SyntecFrameBuilder (报文构建/解析)
  └── SyntecWidgetFactory
        ├── SyntecRealtimeWidget  → pollRequest 信号 → MainWindow → Transport
        ├── SyntecCommandWidget   → sendCommand 信号 → MainWindow → Transport
        └── SyntecParseWidget
```

### 关键设计决策

1. **Transport 层通信** — 标准 TCP，端口默认 5000（新代默认端口）
2. **握手流程** — 连接后发送固定握手包，收到响应后开始轮询
3. **报文格式** — 固定头部（20字节）+ 变长数据区
4. **tid 机制** — 每次请求递增 tid，响应校验 tid 匹配
5. **轮询策略** — 500ms 间隔，串行请求-响应

---

## 报文格式

### 请求包结构

所有请求包共享固定头部格式：

```
偏移  长度  类型    含义
0     4     LE32    包总长度（不含前4字节）
4     4     LE32    头部长度 = 0x10（固定）
8     2     LE16    厂商标识 = 0x00C8
10    1     u8      tid（事务ID）
11    1     u8      保留 = 0
12    4     LE32    厂商标识2 = 0x000000C8
16    4     LE32    功能码
20    N     bytes   数据参数（变长）
```

### 响应包结构

```
偏移  长度  类型    含义
0     4     LE32    包总长度（不含前4字节）
4     4     LE32    厂商标识 = 0x000000C8
8     2     LE16    厂商标识2 = 0x00C8
10    1     u8      tid（与请求匹配）
11    1     u8      保留
12    4     LE32    错误码1（0=成功）
16    4     LE32    错误码2（0=成功）
20    N     bytes   数据内容
```

### 响应校验规则

1. 包长度 >= content_len + 4 + 12
2. 首个4字节 == content_len + 4
3. data[10] == tid（与请求匹配）
4. 错误码1 和 错误码2 都为 0

### 空响应处理

连续收到 `00 00 00 00 FF FF 00 00 00 00 00 00`（12字节全0/FF）最多10次视为断开。

---

## 命令编码

### 握手

| cmdCode | 名称 | 包字节（十六进制） | 响应 |
|---------|------|-------------------|------|
| 0x00 | 握手 | `14 00 00 00 10 00 00 00 C8 00 00 00 C8 00 00 00 4F 04 00 00 04 00 00 00 50 00 00 00 01 00 00 00` | 固定响应 |

### 状态/信息类

| cmdCode | 名称 | 包字节 | 响应 content_len | 数据解析 |
|---------|------|--------|-----------------|---------|
| 0x01 | 程序名 | `14 00 00 00 10 00 00 00 C8 00 01 00 C8 00 00 00 8C 04 00 00 04 00 00 00 04 02 00 00 01 00 00 00` | 0x204 | 偏移20读取字符串（每2字节取1字节直到0x00） |
| 0x02 | 操作模式 | `14 00 00 00 10 00 00 00 C8 00 02 00 C8 00 00 00 07 04 00 00 04 00 00 00 08 00 00 00 05 00 00 00` | 0x8 | 偏移20读取 int：0=NULL,1=EDIT,2=AUTO,3=MDI,4=JOG,5=INCJOG,6=MPG,7=HOME |
| 0x03 | 运行状态 | `14 00 00 00 10 00 00 00 C8 00 03 00 C8 00 00 00 07 04 00 00 04 00 00 00 08 00 00 00 04 00 00 00` | 0x8 | 偏移20读取 int：0=NOTREADY,1=READY,2=START,3=FEEDHOLD,4=STOP |
| 0x04 | 报警 | `10 00 00 00 10 00 00 00 C8 00 04 00 C8 00 00 00 4A 04 00 00 00 00 00 00 08 00 00 00` | 0x8 | 偏移20读取 int = 报警个数 |
| 0x05 | 急停 | `18 00 00 00 10 00 00 00 C8 00 05 00 C8 00 00 00 0C 04 00 00 08 00 00 00 09 00 00 00 24 00 00 00 01 00 00 00` | 0x9 | 偏移24字节：0xFF=急停,其他=正常 |

### 时间类

| cmdCode | 名称 | 包字节 | 响应 | 数据解析 |
|---------|------|--------|------|---------|
| 0x06 | 开机时间 | `14 00 00 00 10 00 00 00 C8 00 06 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 F4 03 00 00` | 0x8 | int，单位秒 |
| 0x07 | 切削时间 | `14 00 00 00 10 00 00 00 C8 00 07 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 F3 03 00 00` | 0x8 | int，单位秒 |
| 0x08 | 循环时间 | `14 00 00 00 10 00 00 00 C8 00 08 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 F2 03 00 00` | 0x8 | int，单位秒 |

### 计数类

| cmdCode | 名称 | 包字节 | 响应 | 数据解析 |
|---------|------|--------|------|---------|
| 0x09 | 工件数 | `14 00 00 00 10 00 00 00 C8 00 0B 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 E8 03 00 00` | 0x8 | int |
| 0x0A | 需求工件数 | `14 00 00 00 10 00 00 00 C8 00 0C 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 EA 03 00 00` | 0x8 | int |
| 0x0B | 总工件数 | `14 00 00 00 10 00 00 00 C8 00 0D 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 EC 03 00 00` | 0x8 | int |

### 倍率/速率类

| cmdCode | 名称 | 包字节 | 响应 | 数据解析 |
|---------|------|--------|------|---------|
| 0x0C | 进给倍率 | `14 00 00 00 10 00 00 00 C8 00 0E 00 C8 00 00 00 07 04 00 00 04 00 00 00 08 00 00 00 13 00 00 00` | 0x8 | int（百分比） |
| 0x0D | 主轴倍率 | `14 00 00 00 10 00 00 00 C8 00 0F 00 C8 00 00 00 07 04 00 00 04 00 00 00 08 00 00 00 15 00 00 00` | 0x8 | int（百分比） |
| 0x0E | 主轴速度 | `14 00 00 00 10 00 00 00 C8 00 10 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 03 03 00 00` | 0x8 | int（RPM） |
| 0x0F | 进给速度(原始) | `14 00 00 00 10 00 00 00 C8 00 11 00 C8 00 00 00 1A 04 00 00 04 00 00 00 08 00 00 00 BC 02 00 00` | 0x8 | int |
| 0x10 | 小数位数 | `14 00 00 00 10 00 00 00 C8 00 12 00 C8 00 00 00 07 04 00 00 04 00 00 00 08 00 00 00 0C 00 00 00` | 0x8 | int |

进给速度 = 进给速度(原始) / 10^小数位数

### 坐标类

| cmdCode | 名称 | 包字节 | 响应 | 数据解析 |
|---------|------|--------|------|---------|
| 0x20 | 相对坐标 | `18 00 00 00 10 00 00 00 C8 00 1F 00 C8 00 00 00 05 04 00 00 08 00 00 00 14 00 00 00 8D 00 00 00 03 00 00 00` | 变长 | 3轴，每轴8字节 int（需除以10^小数位数） |
| 0x21 | 机床坐标 | `18 00 00 00 10 00 00 00 C8 00 1C 00 C8 00 00 00 05 04 00 00 08 00 00 00 14 00 00 00 65 00 00 00 03 00 00 00` | 变长 | 3轴，每轴8字节 int |

注：坐标参数最后2字节 `03 00 00 00` 表示3个轴。每轴数据为 int64（8字节），需除以 10^DecPoint 得到实际坐标值。

---

## SyntecFrameBuilder

### 职责

- 构建各命令的请求包（硬编码字节序列）
- 解析响应包（校验 + 提取数据）
- 状态/模式映射

### 关键方法

```cpp
class SyntecFrameBuilder {
public:
    // 构建请求包
    static QByteArray buildHandshake();
    static QByteArray buildRequest(quint8 cmdCode);

    // 解析响应
    static bool checkFrame(const QByteArray& frame, int contentLen, quint8 expectedTid);
    static int extractInt(const QByteArray& frame, int offset);
    static QString extractString(const QByteArray& frame, int offset);

    // 状态映射
    static QString statusToString(int status);
    static QString statusColor(int status);
    static QString modeToString(int mode);
    static QString emgToString(int emg);
};
```

### 状态映射

- 运行状态: 0=未就绪, 1=就绪, 2=运行中, 3=暂停, 4=停止
- 操作模式: 0=NULL, 1=编辑, 2=自动, 3=MDI, 4=手动, 5=增量, 6=手轮, 7=回零

---

## SyntecProtocol（IProtocol）

### IProtocol 方法

| 方法 | 实现 |
|------|------|
| buildRequest(cmdCode) | 返回对应命令的硬编码字节包 |
| parseResponse(frame) | 校验帧 + 提取 tid 和数据 |
| extractFrame(buffer) | 根据前4字节长度字段提取完整帧 |
| allCommands() | 返回所有命令定义 |
| interpretData() | 格式化显示 |

### extractFrame 逻辑

1. 读取 buffer 前4字节作为 totalLen（小端 int）
2. 如果 buffer.size() < totalLen + 4，返回空（等待更多数据）
3. 提取 buffer[0..totalLen+4] 作为一帧
4. 从 buffer 中移除已提取的数据

---

## SyntecRealtimeWidget

### 轮询策略

间隔 500ms，依次请求：

1. 握手（连接时一次性）
2. getStatus → 更新运行状态
3. getMode → 更新操作模式
4. getProgName → 更新程序名
5. getAlarm → 更新报警
6. getEMG → 更新急停状态
7. getRelativeAxes + getMachineAxes → 更新坐标表（先获取 DecPoint）
8. getFeedOverride + getSpindleOverride → 更新倍率
9. getSpindleSpeed → 更新主轴速度
10. getFeedRateOri + getDecPoint → 计算并更新进给速度
11. getRunTime + getCutTime + getCycleTime → 更新时间
12. getProducts → 更新工件数

使用 pollStep 状态机，每次收到响应后发下一个请求。

### 布局（3+1 行，与 Mazak 一致）

- 第 1 行：运行状态 | 操作模式 | 程序名 | 急停 | 报警 | 工件数
- 第 2 行：进给速度 | 进给倍率 | 主轴速度 | 主轴倍率
- 第 3 行：坐标表（轴 | 机床坐标 | 相对坐标）
- 底部：开机时间 | 切削时间 | 循环时间

---

## SyntecCommandWidget

### Tab 页

**1. 操作**
- 急停复位按钮
- 报警复位按钮
- 各时间清零按钮

注：源码中未提供写入命令，CommandWidget 主要提供操作类功能。如果后续有写入协议文档可扩展。

---

## SyntecParseWidget

- 输入十六进制数据
- 选择数据类型（运行状态、操作模式、报警、急停、时间、工件数、倍率、速度、坐标等）
- 解析并显示结构化结果

---

## MainWindow 集成

### switchProtocol() 新增 case

```cpp
case 6:
    m_protocol = new SyntecProtocol(this);
    m_widgetFactory = new SyntecWidgetFactory;
    break;
```

### 协议下拉框

```cpp
m_protocolCombo->addItem("Syntec 新代");
```

### 连接流程

1. 用户选择 Syntec 协议，点击连接
2. Transport（TCP）连接到目标 IP:5000
3. 连接成功后发送握手包
4. 收到握手响应后启动轮询

### Mock 模式

- Mock 服务器返回预设的响应帧
- 各命令的 mock 响应在 SyntecProtocol::mockResponse() 中定义

---

## 构建系统

### GSK988Tool.pro 添加

```
src/protocol/syntec/syntecframebuilder.h/cpp
src/protocol/syntec/syntecprotocol.h/cpp
src/protocol/syntec/syntecwidgetfactory.h/cpp
src/protocol/syntec/syntecrealtimewidget.h/cpp
src/protocol/syntec/synteccommandwidget.h/cpp
src/protocol/syntec/syntecparsewidget.h/cpp
```

---

## 风险和待定项

1. **坐标解析** — 文档中坐标数据是 int64（8字节每轴），需确认是否需要 DecPoint 换算
2. **默认端口** — 假设为 5000，需确认实际端口
3. **写入命令** — 源码中只有读取命令，写入命令待后续协议文档补充
4. **多轴支持** — 坐标命令参数中轴数=3，可能需要支持更多轴
