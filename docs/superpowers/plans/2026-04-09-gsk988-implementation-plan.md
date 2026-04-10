# GSK988 调试工具 v2.0 — 实施计划

**日期：** 2026-04-09
**设计文档：** `docs/superpowers/specs/2026-04-09-gsk988-tcp-debug-tool-design.md`

---

## Phase 1: 项目基础搭建

### 1.1 创建 .pro 文件 (GSK988Tool.pro)

```pro
QT += core gui network widgets
TARGET = GSK988Tool
TEMPLATE = app
CONFIG += c++14

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/network/tcpclient.cpp \
    src/network/mockserver.cpp \
    src/protocol/gsk988protocol.cpp \
    src/protocol/framebuilder.cpp \
    src/ui/realtimewidget.cpp \
    src/ui/commandwidget.cpp \
    src/ui/parsewidget.cpp \
    src/ui/logwidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/network/tcpclient.h \
    src/network/mockserver.h \
    src/protocol/gsk988protocol.h \
    src/protocol/framebuilder.h \
    src/ui/realtimewidget.h \
    src/ui/commandwidget.h \
    src/ui/parsewidget.h \
    src/ui/logwidget.h
```

### 1.2 创建 main.cpp

最小化入口：
- 创建 QApplication
- 创建 MainWindow
- show() + exec()

### 1.3 创建目录结构

```
src/
src/network/
src/protocol/
src/ui/
```

---

## Phase 2: 协议层 (Protocol)

### 2.1 FrameBuilder (framebuilder.h/.cpp)

底层帧构建工具，不知道命令语义，只做帧拼接。

**接口：**
```cpp
class FrameBuilder {
public:
    // 构建请求帧
    static QByteArray buildRequestFrame(const QByteArray& dataField);
    // 构建响应帧
    static QByteArray buildResponseFrame(const QByteArray& dataField);
    // 从接收缓冲区提取一帧完整数据
    static QByteArray extractFrame(QByteArray& buffer);
    // 校验帧完整性（帧头、帧尾、长度一致性）
    static bool validateFrame(const QByteArray& frame);

    static constexpr quint8 FRAME_HEAD[] = {0x93, 0x00};
    static constexpr quint8 FRAME_TAIL[] = {0x55, 0xAA};
    // 注意：QByteArray 不是 literal type，不能用 constexpr
    static const QByteArray REQUEST_ID;
    static const QByteArray RESPONSE_ID;
    // 在 .cpp 中初始化：
    // const QByteArray FrameBuilder::REQUEST_ID = QByteArray::fromHex("6fc81e641e171017");
    // const QByteArray FrameBuilder::RESPONSE_ID = QByteArray::fromHex("00641ec81e171001");
};
```

**帧格式说明（与设计文档 3.1 节对齐）：**
```
完整帧 = 帧头(2B) + 长度(2B) + 标识(8B) + 数据域(NB) + 帧尾(2B)
长度字段 = 标识(8B) + 数据域(NB) 的总字节数，大端序 quint16
```
- 注意：长度字段包含标识的 8 字节，不仅只是数据域
- 数据域（请求帧）= 命令码(1B) + 参数(变长)，无错误码
- 数据域（响应帧）= 命令码(1B) + 错误码(2B) + 附加数据(变长)

**帧提取逻辑（extractFrame）：**
1. 查找 `0x93 0x00` 帧头位置
2. 帧头后 2 字节为长度 N（大端序）
3. 完整帧 = 帧头(2B) + 长度(2B) + N 字节数据 + 帧尾(2B) = 6+N
4. 缓冲区不足时返回空，等待更多数据
5. 校验帧尾是否为 `0x55 0xAA`

### 2.2 Gsk988Protocol (gsk988protocol.h/.cpp)

协议核心，知道命令定义和语义。

**接口：**
```cpp
struct CommandDef {
    quint8 cmdCode;
    QString name;
    QString category;       // "通讯管理"/"实时信息"/"参数"/"刀补"等
    QString requestParams;  // 参数描述
    QString responseDesc;   // 响应描述
};

struct ParsedResponse {
    quint8 cmdCode;
    quint16 errorCode;
    QByteArray rawData;     // 原始附加数据（不含命令码和错误码）
    bool isValid;
    QString errorString;    // 可读错误信息
};

class Gsk988Protocol : public QObject {
    Q_OBJECT
public:
    explicit Gsk988Protocol(QObject* parent = nullptr);

    // 构建请求帧
    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {});

    // 解析响应帧
    ParsedResponse parseResponse(const QByteArray& frame);

    // 获取所有命令定义
    static QVector<CommandDef> allCommands();

    // 根据命令码获取定义
    static CommandDef commandDef(quint8 cmdCode);

    // 语义解析：根据命令码解释附加数据
    static QString interpretData(quint8 cmdCode, const QByteArray& data);

    // 生成 Mock 响应数据（不含帧头帧尾，只有数据域部分）
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);
};
```

**命令定义表实现：**
- 用静态 QVector<CommandDef> 存储所有命令定义（从设计文档 3.3 节）
- 每个命令的 cmdCode、name、category 都硬编码

**buildRequest 逻辑：**
1. 数据域 = 命令码(1B) + params（注意：请求帧数据域无错误码字段）
2. 调用 FrameBuilder::buildRequestFrame(dataField)

**parseResponse 逻辑：**
1. 调用 FrameBuilder::validateFrame 校验
2. 跳过帧头(2B) + 长度(2B) + 应答标识(8B)，进入数据域
3. 响应帧数据域 = 命令码(1B) + 错误码(2B) + 附加数据(变长)
4. 提取命令码(1B) + 错误码(2B, quint16 大端序) + 剩余为 rawData
5. 错误码非零时设置 isValid=false，填充 errorString

**interpretData 逻辑（按命令码分支）：**
- 0x0A: 解析权限值
- 0x0B: 解析字符串
- 0x10~0x24: 按设计文档解析各字段
- 0x21: 解析轴数 + 坐标列表
- 0x81: 解析错误数+警告数
- 其他: 返回 HEX 原始数据

**特殊命令参数说明：**
- 0x1A（速度参数）：请求参数为 速度类型(1B) + 轴号(1B) + 通道号(1B)，通道号在最后（协议定义如此，与其他命令通道号在前不同）

**mockResponseData 逻辑：**
- 按设计文档 4.3 节的 Mock 默认数据表返回
- 命令码(1B) + 错误码(0x0000) + mock数据

---

## Phase 3: 网络层 (Network)

### 3.1 TcpClient (tcpclient.h/.cpp)

TCP 客户端，异步非阻塞，通过信号与上层通信。

**接口：**
```cpp
class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);
    ~TcpClient();

    void connectTo(const QString& ip, quint16 port);
    void disconnect();
    bool isConnected() const;
    void sendFrame(const QByteArray& frame);

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& msg);
    void responseReceived(const QByteArray& frame);  // 已提取的完整帧
    void responseTimeout();

private slots:
    void onReadyRead();
    void onTimeout();

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;         // 接收缓冲区
    QTimer* m_timeoutTimer;      // 3秒超时定时器
    QByteArray m_pendingFrame;   // 等待响应的请求帧（用于超时日志）
};
```

**关键实现要点：**
1. **连接管理：** connectTo 调用 socket->connectToHost()，监听 connected/error 信号
2. **数据接收：** onReadyRead 中将数据追加到 m_buffer，循环调用 FrameBuilder::extractFrame 提取完整帧，每提取一帧发出 responseReceived 信号
3. **发送：** sendFrame 直接写 socket，同时启动 3 秒超时定时器
4. **超时：** QTimer 触发后发出 responseTimeout 信号，停止定时器
5. **断线检测：** 监听 socket 的 disconnected 信号，清理 buffer 和定时器

### 3.2 MockServer (mockserver.h/.cpp)

内嵌 Mock TCP 服务器，监听本地端口，自动响应请求。

**接口：**
```cpp
class MockServer : public QObject {
    Q_OBJECT
public:
    explicit MockServer(QObject* parent = nullptr);
    ~MockServer();

    bool start(quint16 port = 6000);
    void stop();
    bool isRunning() const;
    quint16 serverPort() const;

signals:
    void logMessage(const QString& msg);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
    Gsk988Protocol* m_protocol;  // 用于解析请求和生成响应
};
```

**关键实现要点：**
1. **启动：** m_server->listen(QHostAddress::LocalHost, port)
2. **接收连接：** newConnection 信号 → 获取 socket → 加入 m_clients → 监听 readyRead
3. **处理请求：** 从 socket 读数据 → extractFrame 提取完整帧 → 解析命令码 → 调用 Gsk988Protocol::mockResponseData 生成数据 → FrameBuilder::buildResponseFrame 封装 → 写回 socket
4. **客户端清理：** 监听 socket disconnected 信号，从 m_clients 移除
5. **日志：** 每次 request/response 都发出 logMessage 信号

---

## Phase 4: 主窗口 (MainWindow)

### 4.1 MainWindow (mainwindow.h/.cpp)

主窗口组装工具栏和 Tab 页，协调各模块数据流。

**接口：**
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onResponseReceived(const QByteArray& frame);
    void onConnectionStatusChanged();

private:
    void setupToolBar();
    void setupTabs();
    void setupConnections();
    void updateConnectionIndicator(ConnectionState state);

    enum ConnectionState { Disconnected, Connected, Error };

    // 核心对象
    Gsk988Protocol* m_protocol;
    TcpClient* m_client;
    MockServer* m_mockServer;

    // 工具栏控件
    QLineEdit* m_ipEdit;
    QSpinBox* m_portSpin;
    QComboBox* m_modeCombo;      // "真实设备" / "Mock模式"
    QPushButton* m_connectBtn;
    QLabel* m_statusIndicator;   // 状态圆点
    QLabel* m_statusLabel;       // 状态文字

    // Tab 页
    QTabWidget* m_tabWidget;
    RealtimeWidget* m_realtimeTab;
    CommandWidget* m_commandTab;
    ParseWidget* m_parseTab;
    LogWidget* m_logTab;
};
```

**工具栏布局（setupToolBar）：**
1. 创建 QToolBar，设为不可拖动（Movable = false）
2. 添加 IP 输入框（默认 "192.168.1.100"，宽度 130px）
3. 添加端口 SpinBox（范围 1-65535，默认 6000）
4. 添加模式下拉（"真实设备" / "Mock模式"）
5. 添加连接按钮（文字随状态切换："连接" / "断开"）
6. 添加状态指示器（QLabel，用 QSS 设圆形背景色：灰/绿/红）

**连接逻辑（onConnectClicked）：**
1. 读取模式选择
2. Mock 模式：先启动 MockServer，再将 IP 设为 127.0.0.1
3. 真实模式：直接使用用户输入的 IP
4. 调用 TcpClient::connectTo(ip, port)
5. 连接成功 → 按钮变"断开"，状态变绿，启动实时轮询
6. 断开 → 按钮变"连接"，状态变灰，停止轮询

**数据流协调：**
- TcpClient::responseReceived → MainWindow::onResponseReceived
  - 解析响应（Gsk988Protocol::parseResponse）
  - 分发到当前活跃 Tab 和 LogWidget
  - 如果是实时轮询命令的响应，更新 RealtimeWidget
- Tab 页发送命令 → 通过 MainWindow 调用 TcpClient::sendFrame
- 所有收发帧都转发到 LogWidget 记录

---

## Phase 5: UI Tab 页

### 5.1 RealtimeWidget (realtimewidget.h/.cpp)

实时数据展示，2×2 卡片布局 + 底部 HEX 显示。

**接口：**
```cpp
class RealtimeWidget : public QWidget {
    Q_OBJECT
public:
    explicit RealtimeWidget(QWidget* parent = nullptr);

    // 接收解析后的响应数据，按命令码更新对应卡片
    void updateData(const ParsedResponse& resp);

    // 设置轮询间隔（秒）
    void setPollInterval(int seconds);

    // 启停轮询
    void startPolling();
    void stopPolling();

signals:
    // 轮询触发时发出，请求 MainWindow 发送对应命令
    void pollRequest(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void updateDeviceCard(const ParsedResponse& resp);    // 设备状态
    void updateCoordCard(const ParsedResponse& resp);     // 机械坐标
    void updateMachiningCard(const ParsedResponse& resp); // 加工信息
    void updateAlarmCard(const ParsedResponse& resp);     // 报警
    void appendHexDisplay(const QByteArray& data, bool isSend);

    // 卡片
    QGroupBox* m_deviceCard;
    QGroupBox* m_coordCard;
    QGroupBox* m_machiningCard;
    QGroupBox* m_alarmCard;

    // 卡片内标签
    QLabel* m_runModeLabel;
    QLabel* m_runStatusLabel;
    QLabel* m_coordLabels[3];      // X, Y, Z
    QLabel* m_pieceCountLabel;
    QLabel* m_runTimeLabel;
    QLabel* m_cutTimeLabel;
    QLabel* m_toolNoLabel;
    QLabel* m_errorCountLabel;
    QLabel* m_warnCountLabel;

    QTextEdit* m_hexDisplay;       // 底部 HEX 显示

    QTimer* m_pollTimer;
    int m_pollIndex;               // 当前轮询到哪个命令
};
```

**布局（setupUI）：**
1. 上方用 QGridLayout(2×2) 放 4 个 QGroupBox 卡片
2. 每个卡片内用 QFormLayout 放 QLabel 键值对
3. 下方 QTextEdit（只读，等宽字体）显示原始 HEX

**轮询逻辑（请求-应答同步，避免错位）：**
- 轮询命令队列：[0x11, 0x21, 0x16, 0x17, 0x18, 0x19, 0x81]
- m_pollTimer 间隔 300ms，但**只在收到上一个响应（或超时）后才发下一个**
- 具体流程：
  1. m_pollTimer 触发 → 发出 pollRequest(m_commands[m_pollIndex], {})
  2. 设置 m_waitingResponse = true，**停止定时器**
  3. MainWindow 收到 responseReceived → 调用 RealtimeWidget::updateData()
  4. updateData() 中检查 m_waitingResponse → 设置 false，m_pollIndex++，**重启定时器**
  5. 若超时（TcpClient 发出 responseTimeout）→ 同样重置状态，推进 index
- 总轮询周期：7 命令 × 300ms ≈ 2.1 秒（实际取决于网络延迟）
- 新增成员变量：`bool m_waitingResponse = false;`

### 5.2 CommandWidget (commandwidget.h/.cpp)

所有读取命令列表，支持筛选和发送。

**接口：**
```cpp
class CommandWidget : public QWidget {
    Q_OBJECT
public:
    explicit CommandWidget(QWidget* parent = nullptr);

    // 显示命令发送结果
    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void populateTable();
    void applyFilter();

    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QTableWidget* m_table;
    QTextEdit* m_resultDisplay;    // 发送结果显示区

    QVector<CommandDef> m_commands; // 所有命令定义
};
```

**表格列（populateTable）：**
| 列 | 内容 |
|---|---|
| 0 | 类型标签（彩色 QLabel，按 category 着色） |
| 1 | 命令码（"0x0A" 格式） |
| 2 | 命令名称 |
| 3 | 通道输入（QSpinBox，不需要通道的命令此列为空） |
| 4 | 参数输入（QLineEdit，不需要参数的命令此列为空） |
| 5 | 发送按钮（QPushButton） |

**筛选逻辑（applyFilter）：**
1. 获取搜索文本和类别选择
2. 遍历表格所有行，匹配命令名/命令码/类别
3. 隐藏不匹配的行

**发送逻辑：**
- 点击发送按钮 → 从该行读取命令码、通道号、参数 → 组装 QByteArray → 发出 sendCommand 信号

### 5.3 ParseWidget (parsewidget.h/.cpp)

手动输入 HEX 帧进行解析验证。

**接口：**
```cpp
class ParseWidget : public QWidget {
    Q_OBJECT
public:
    explicit ParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;       // HEX 输入框
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;  // 解析结果，带颜色标注

    Gsk988Protocol* m_protocol;
};
```

**解析逻辑（doParse）：**
1. 从 m_hexInput 读取文本，去除空格、逗号、0x 前缀
2. 转为 QByteArray
3. 判断是请求帧还是响应帧（检查标识字段）
4. 逐步提取各字段，在结果区用 HTML 着色显示：
   - 帧头：蓝色
   - 长度：灰色
   - 标识：橙色
   - 命令码：绿色
   - 错误码：红色（非零时）
   - 附加数据：紫色
   - 帧尾：蓝色
5. 如果是响应帧，调用 interpretData 显示语义解析

### 5.4 LogWidget (logwidget.h/.cpp)

通讯日志记录，实时显示所有收发数据。

**接口：**
```cpp
class LogWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);

public slots:
    // 记录一条日志
    void logFrame(const QByteArray& frame, bool isSend, const QString& cmdDesc = {});
    // 记录错误/超时
    void logError(const QString& msg);
    // 清空日志
    void clearLog();
    // 导出日志
    void exportLog();

private:
    void setupUI();

    QComboBox* m_filterCombo;    // 全部/发送/接收/错误
    QTextEdit* m_logDisplay;     // 日志显示（等宽字体，只读）
    QPushButton* m_clearBtn;
    QPushButton* m_exportBtn;

    struct LogEntry {
        QDateTime time;
        enum Type { Send, Recv, Error } type;
        QString message;
        QByteArray rawData;
    };
    QVector<LogEntry> m_entries;
};
```

**日志格式：**
```
[14:30:25.123] [发送] [0x21] 当前坐标
93 00 00 09 6F C8 1E 64 1E 17 10 17 21 00 55 AA

[14:30:25.145] [接收] [0x21] 当前坐标 — 成功
93 00 00 2D 00 64 1E C8 1E 17 10 01 21 00 00 ...
  → X=123.456, Y=-45.678, Z=0.000
```

**筛选逻辑：** 根据 m_filterCombo 选择，过滤 m_entries 后刷新显示。

**导出逻辑：** QFileDialog 选择保存路径，将 m_entries 格式化写入 TXT 文件。

---

## 实施顺序总结

| 步骤 | 内容 | 依赖 |
|------|------|------|
| 1 | 创建 .pro + main.cpp + 目录结构 | 无 |
| 2 | FrameBuilder（帧构建/提取/校验） | 无 |
| 3 | Gsk988Protocol（命令定义+请求构建+响应解析+Mock数据） | FrameBuilder |
| 4 | TcpClient | FrameBuilder |
| 5 | MockServer | Gsk988Protocol + FrameBuilder |
| 6 | LogWidget | 无 |
| 7 | RealtimeWidget | 无（接收 ParsedResponse） |
| 8 | CommandWidget | 无（发出信号） |
| 9 | ParseWidget | Gsk988Protocol |
| 10 | MainWindow（组装所有模块） | 所有以上 |
| 11 | 编译调试 | 全部 |

每步完成后可单独编译验证，确保无编译错误再进行下一步。
