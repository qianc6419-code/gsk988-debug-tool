# Mazak Smooth 协议集成 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 Mazak Smooth CNC 协议集成到多协议调试工具，通过 NTIFDLL.dll 实现实时监控、命令下发和数据解析。

**Architecture:** 使用 QLibrary 加载 NTIFDLL.dll，MazakDLLWrapper 单例封装所有 DLL 函数。MazakProtocol 适配 IProtocol 接口但不走 Transport 层。三件套 Widget 直接调用 DLL 获取/写入数据。

**Tech Stack:** Qt 5/6 (Widgets, Network), C++14, QLibrary 动态加载

**Spec:** `docs/superpowers/specs/2026-04-13-mazak-smooth-protocol-design.md`

**参考 C# 源码:** `C:\Users\QC\Desktop\mazaksmooth\mazaksmooth\` 下的 NTIFStruct.cs、MazakTCPLib.cs、Form1.cs

---

## File Structure

| 文件 | 职责 |
|------|------|
| `src/protocol/mazak/mazakframebuilder.h/cpp` | 数据结构定义（从 NTIFStruct.cs 移植），DLL 错误码，辅助转换函数 |
| `src/protocol/mazak/mazakdllwrapper.h/cpp` | QLibrary 加载 NTIFDLL.dll，单例，绑定 DLL 导出函数为类型安全 C++ 方法 |
| `src/protocol/mazak/mazakprotocol.h/cpp` | 实现 IProtocol，命令定义，DLL 调用适配 |
| `src/protocol/mazak/mazakwidgetfactory.h/cpp` | 创建三件套 Widget |
| `src/protocol/mazak/mazakrealtimewidget.h/cpp` | 实时监控页：状态、坐标、主轴、进给、报警 |
| `src/protocol/mazak/mazakcommandwidget.h/cpp` | 命令下发页：参数/刀具/坐标/程序 Tab |
| `src/protocol/mazak/mazakparsewidget.h/cpp` | 数据解析工具 |
| `GSK988Tool.pro` | 添加新文件到构建 |
| `src/mainwindow.h/cpp` | 添加 Mazak 协议注册和连接处理 |

---

### Task 1: 数据结构和常量定义

**Files:**
- Create: `src/protocol/mazak/mazakframebuilder.h`
- Create: `src/protocol/mazak/mazakframebuilder.cpp`

从 NTIFStruct.cs 移植所有结构体到 C++，使用 `#pragma pack(1)` 保证内存对齐。重点关注 RealtimeWidget 和 CommandWidget 需要的核心结构体。

- [ ] **Step 1: 创建 mazakframebuilder.h**

```cpp
#ifndef MAZAKFRAMEBUILDER_H
#define MAZAKFRAMEBUILDER_H

#include <QString>
#include <QByteArray>
#include <cstring>

// DLL 导出函数使用的最大常量
namespace MazakConst {
    constexpr int AXISMAX   = 16;
    constexpr int TOOLMAX   = 20;
    constexpr int ALMMAX    = 24;
    constexpr int ALMHISMAX = 72;
}

// DLL 错误码
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

#pragma pack(push, 1)

// === 核心运行信息 ===

struct MAZ_NCTIME {
    quint32 hor;
    quint32 min;
    quint32 sec;
    char    dummy[4];
};

struct MAZ_NCONTIME {
    quint32 year;
    quint32 mon;
    quint32 day;
    quint32 hor;
    quint32 min;
    quint32 sec;
};

struct MAZ_ACCUM_TIME {
    MAZ_NCTIME power_on;
    MAZ_NCTIME auto_ope;
    MAZ_NCTIME auto_cut;
    MAZ_NCTIME total_cut;
    MAZ_NCTIME total_time;
};

struct MAZ_VERINFO {
    quint16 majorver;
    quint16 minorver;
    quint16 releasever;
    quint16 buildver;
};

struct MTC_ONE_RUNNING_INFO {
    quint8  sts;
    char    dummy[7];
    qint16  ncmode;
    qint16  ncsts;
    qint16  tno;
    quint8  suffix;
    quint8  sufattr;
    qint16  figA;
    qint16  figB;
    qint32  nom;
    qint32  gno;
    qint16  sp_override;
    qint16  ax_override;
    qint16  rpid_override;
    qint16  almno;
    qint32  alm_fore_color;
    qint32  alm_back_color;
    qint16  prtcnt;
    qint16  prtcnt_clamp;
    qint32  autotim;
    qint32  cuttim;
    qint32  totaltim;
    char    dummy3[112];
};

struct MTC_RUNNING_INFO {
    MTC_ONE_RUNNING_INFO run_info[4];
    MTC_ONE_RUNNING_INFO dummy[2];
};

struct MTC_ONE_AXIS_INFO {
    quint8  sts;
    quint8  dummy;
    char    axis_name[4];
    char    dummy1[2];
    double  pos;
    double  mc_pos;
    double  load;
    double  feed;
    double  dummy2[17];
};

struct MTC_COMP_FEED_INFO {
    quint8  sts;
    char    dummy[7];
    double  feed;
};

struct MTC_AXIS_INFO {
    MTC_COMP_FEED_INFO  composited_feedrate[4];
    MTC_ONE_AXIS_INFO   axis[16];
    MTC_ONE_AXIS_INFO   reserved[8];
};

struct MTC_ONE_SPINDLE_INFO {
    quint8  sts;
    quint8  type;
    char    dummy[6];
    double  rot;
    double  load;
    double  temp;
    double  dummy2[17];
};

struct MTC_SPINDLE_INFO {
    MTC_ONE_SPINDLE_INFO sp_info[8];
    MTC_ONE_SPINDLE_INFO dummy[4];
};

// === 坐标 ===

struct MAZ_NCPOS {
    char    status[16];
    qint32  data[16];
};

// === 进给 ===

struct MAZ_FEED {
    qint32 fmin;
    qint32 frev;
};

// === 报警 ===

struct MAZ_ALARM {
    qint16 eno;
    quint8 sts;
    char   dummy[5];
    char   pa1[32];
    char   pa2[16];
    char   pa3[16];
    quint8 mon;
    quint8 day;
    quint8 hor;
    quint8 min;
    char   dummy2[4];
    char   extmes[32];
    char   head[4];
    char   dummy3[4];
    char   message[64];
};

struct MAZ_ALARMALL {
    MAZ_ALARM alarm[24];
};

struct MAZ_ALARMHIS {
    MAZ_ALARM alahis[72];
};

// === 程序 ===

struct MAZ_PROINFO {
    char   wno[33];
    char   dummy[7];
    char   comment[49];
    char   dummy2[7];
    quint8 type;
    char   dummy3[7];
};

struct MAZ_PROINFO2 {
    char   wno[33];
    char   dummy[7];
    char   comment[49];
    char   dummy2[7];
    quint8 type;
    char   dummy3[7];
    qint32 uno;
    char   dummy4[4];
    qint32 sno;
    char   dummy5[4];
    qint32 bno;
    char   dummy6[4];
};

struct MTC_ONE_PROGRAM_INFO {
    quint8 sts;
    char   dummy[7];
    qint32 wno;
    qint32 subwno;
    qint32 blockno;
    qint32 seqno;
    qint32 unitno;
    char   wno_string[33];
    char   subwno_string[33];
    char   wno_comment[49];
    char   subwno_comment[49];
    char   dummy3[44];
};

struct MTC_PROGRAM_INFO {
    MTC_ONE_PROGRAM_INFO prog_info[4];
    MTC_ONE_PROGRAM_INFO dummy[2];
};

// === 轴名/负载 ===

struct MAZ_AXISNAME {
    char status[16];
    char axisname[64];
};

struct MAZ_AXISLOAD {
    char    status[16];
    quint16 data[16];
};

// === 刀具 ===

struct MAZ_TOOLINFO {
    quint16 tno;
    quint8  suf;
    quint8  sufatr;
    quint8  name;
    quint8  part;
    quint8  sts;
    char    dummy[1];
    qint32  yob;
    char    dummy2[4];
};

struct MAZ_TDCOMP {
    MAZ_TOOLINFO info;
    quint32 sts;
    qint32  toolsetX;
    qint32  toolsetZ;
    qint32  offsetX;
    qint32  offsetY;
    qint32  offsetZ;
    qint32  lengthA;
    qint32  lengthB;
    qint32  radius;
    qint32  LenOffset;
    qint32  RadOffset;
    qint32  LenCompno;
    qint32  RadCompno;
    char    dummy[4];
};

struct MAZ_TDCOMPALL {
    MAZ_TDCOMP offset[20];
};

struct MAZ_TLIFE {
    MAZ_TOOLINFO info;
    quint32 sts;
    quint32 lif;
    quint32 use;
    quint32 clif;
    quint32 cuse;
    char    dummy[4];
};

struct MAZ_TLIFEALL {
    MAZ_TLIFE toolLife[20];
};

struct MAZ_TOFFCOMP {
    qint32 type;
    qint32 offset1;
    qint32 offset2;
    qint32 offset3;
    qint32 offset4;
    qint32 offset5;
    qint32 offset6;
    qint32 offset7;
    qint32 offset8;
    qint32 offset9;
};

// === NC 版本 ===

struct MAZ_NC_VERINFO {
    char szMainA[17];    char dummy1[7];
    char szMainB[17];    char dummy2[7];
    char szLadder[17];   char dummy3[7];
    char szUnitName[17]; char dummy4[7];
    char szNCSerial[17]; char dummy5[7];
    char szNCModel[17];  char dummy6[7];
};

// === 维护 ===

struct MAZ_MAINTE {
    char    lpszComment[49];
    char    dummy[7];
    quint16 uYear;
    quint16 uMonth;
    quint16 uDay;
    qint16  nTargetTime;
    qint32  lProgressSec;
};

struct MAZ_LMAINTE {
    char    lpszComment[1568];
    char    dummy[4];
    quint16 uYear;
    quint16 uMonth;
    quint16 uDay;
    qint16  nTargetTime;
    qint32  lProgressSec;
    quint32 flgReversed;
};

struct MAZ_MAINTE_CHECK {
    MAZ_MAINTE  mainte[22];
    MAZ_LMAINTE lmainte[2];
};

#pragma pack(pop)

// 辅助函数
namespace MazakFrameBuilder {
    QString errorToString(int code);
    QString fixedString(const char* data, int maxSize);
}

#endif // MAZAKFRAMEBUILDER_H
```

- [ ] **Step 2: 创建 mazakframebuilder.cpp**

```cpp
#include "protocol/mazak/mazakframebuilder.h"

namespace MazakFrameBuilder {

QString errorToString(int code)
{
    switch (code) {
    case MazakError::OK:        return "成功";
    case MazakError::SOCK:      return "套接字错误";
    case MazakError::HNDL:      return "句柄无效";
    case MazakError::CLIMAX:    return "超过最大客户端数";
    case MazakError::SERVERMAX: return "超过服务器最大连接数";
    case MazakError::VER:       return "版本不匹配";
    case MazakError::BUSY:      return "资源忙";
    case MazakError::RUNNING:   return "NC 正在运行";
    case MazakError::OVER:      return "数据溢出";
    case MazakError::NONE:      return "数据不存在";
    case MazakError::TYPE:      return "数据类型错误";
    case MazakError::EDIT:      return "编辑中";
    case MazakError::PROSIZE:   return "程序大小超限";
    case MazakError::PRONUM:    return "程序号错误";
    case MazakError::ARG:       return "参数错误";
    case MazakError::SYS:       return "系统错误";
    case MazakError::FUNC:      return "功能不可用";
    case MazakError::TIMEOUT:   return "超时";
    default:                    return QString("未知错误(%1)").arg(code);
    }
}

QString fixedString(const char* data, int maxSize)
{
    return QString::fromLocal8Bit(data, static_cast<int>(strnlen(data, maxSize)));
}

} // namespace MazakFrameBuilder
```

- [ ] **Step 3: 编译验证**

Run: 在 GSK988Tool.pro 添加这两个文件后执行构建，确认无编译错误。

- [ ] **Step 4: Commit**

```bash
git add src/protocol/mazak/mazakframebuilder.h src/protocol/mazak/mazakframebuilder.cpp
git commit -m "feat(mazak): add data structures ported from NTIFStruct.cs"
```

---

### Task 2: DLL 封装层

**Files:**
- Create: `src/protocol/mazak/mazakdllwrapper.h`
- Create: `src/protocol/mazak/mazakdllwrapper.cpp`

用 QLibrary 动态加载 NTIFDLL.dll，封装连接管理和核心读取/写入函数。初期只封装 RealtimeWidget 和 CommandWidget 明确需要的函数，其余后续按需添加。

- [ ] **Step 1: 创建 mazakdllwrapper.h**

头文件声明单例类，包含 load/unload、连接管理、按功能分组的读取和写入方法。每个方法内部调用对应的 DLL 函数指针。

关键函数指针 typedef：

```cpp
// 连接
using MazConnectFunc = int(*)(unsigned short*, const char*, int, int);
using MazDisconnectFunc = int(*)(unsigned short);

// 运行信息
using GetRunningInfoFunc = int(*)(unsigned short, MTC_RUNNING_INFO*);
using GetRunningStsFunc = int(*)(unsigned short, unsigned short, short*);
using GetRunModeFunc = int(*)(unsigned short, unsigned short, short*);

// 坐标
using GetAxisInfoFunc = int(*)(unsigned short, MTC_AXIS_INFO*);
using GetCurrentPosFunc = int(*)(unsigned short, MAZ_NCPOS*);
using GetRemainFunc = int(*)(unsigned short, MAZ_NCPOS*);
using GetAxisNameFunc = int(*)(unsigned short, MAZ_AXISNAME*);
using GetAxisLoadFunc = int(*)(unsigned short, MAZ_AXISLOAD*);

// 主轴
using GetSpindleInfoFunc = int(*)(unsigned short, MTC_SPINDLE_INFO*);
using GetSpindleLoadFunc = int(*)(unsigned short, unsigned short, unsigned short*);
using GetCurrentSpindleRevFunc = int(*)(unsigned short, unsigned short, int*);
using GetOrderSpindleRevFunc = int(*)(unsigned short, unsigned short, int*);
using GetSpindleOverRideFunc = int(*)(unsigned short, unsigned short, unsigned short*);

// 进给
using GetFeedFunc = int(*)(unsigned short, unsigned short, MAZ_FEED*);
using GetFeedOverRideFunc = int(*)(unsigned short, unsigned short, unsigned short*);
using GetRapidOverRideFunc = int(*)(unsigned short, unsigned short, unsigned short*);

// 刀具
using GetCurrentToolFunc = int(*)(unsigned short, unsigned short, MAZ_TOOLINFO*);
using GetToolOffsetCompFunc = int(*)(unsigned short, MAZ_TDCOMPALL*);

// 报警
using GetAlarmFunc = int(*)(unsigned short, MAZ_ALARMALL*);

// 程序
using GetMainProFunc = int(*)(unsigned short, unsigned short, MAZ_PROINFO*);
using GetRunningProFunc = int(*)(unsigned short, unsigned short, MAZ_PROINFO*);
using GetProgramInfoFunc = int(*)(unsigned short, MTC_PROGRAM_INFO*);

// 时间
using GetAccumulatedTimeFunc = int(*)(unsigned short, unsigned short, MAZ_ACCUM_TIME*);
using GetNcPowerOnTimeFunc = int(*)(unsigned short, MAZ_NCONTIME*);
using GetRunningTimeFunc = int(*)(unsigned short, unsigned short, MAZ_NCTIME*);
using GetPreparationTimeFunc = int(*)(unsigned short, unsigned short, MAZ_NCTIME*);

// 系统
using GetPartsCountFunc = int(*)(unsigned short, unsigned short, int*);
using GetMachineUnitFunc = int(*)(unsigned short, short*);
using GetClientVerFunc = int(*)(unsigned short, MAZ_VERINFO*);
using GetServerVerFunc = int(*)(unsigned short, MAZ_VERINFO*);
using GetNCVersionInfoFunc = int(*)(unsigned short, MAZ_NC_VERINFO*);

// 参数/寄存器
using GetRangedParamFunc = int(*)(unsigned short, int, int, int*);
using SetRangedParamFunc = int(*)(unsigned short, int, int, int*);
using GetRangedCmnVarFunc = int(*)(unsigned short, short, short, short, double*);
using SetRangedCmnVarFunc = int(*)(unsigned short, short, short, short, double*);
using GetRangedRRegFunc = int(*)(unsigned short, int, int, int*);
using SetRangedRRegFunc = int(*)(unsigned short, int, int, int*);

// 坐标系
using GetRangedWorkCoordSysFunc = int(*)(unsigned short, int, int, double*);
using SetRangedWorkCoordSysFunc = int(*)(unsigned short, int, int, double*);

// 刀补
using GetRangedToolOffsetCompFunc = int(*)(unsigned short, int, int, MAZ_TOFFCOMP*);
using SetRangedToolOffsetCompFunc = int(*)(unsigned short, int, int, MAZ_TOFFCOMP*);

// 复位
using MazResetFunc = int(*)(unsigned short);
```

公共方法签名：

```cpp
class MazakDLLWrapper {
public:
    static MazakDLLWrapper& instance();

    bool load(const QString& dllPath);
    bool isLoaded() const;
    void unload();

    // 连接管理
    bool connect(unsigned short& handle, const QString& ip, int port, int timeout);
    bool disconnect(unsigned short handle);
    bool isConnected() const { return m_connected; }
    unsigned short handle() const { return m_handle; }

    // === 读取函数 ===
    bool getRunningInfo(MTC_RUNNING_INFO& out);
    bool getRunningSts(unsigned short path, short& out);
    bool getRunMode(unsigned short path, short& out);
    bool getAxisInfo(MTC_AXIS_INFO& out);
    bool getCurrentPos(MAZ_NCPOS& out);
    bool getRemain(MAZ_NCPOS& out);
    bool getAxisName(MAZ_AXISNAME& out);
    bool getAxisLoad(MAZ_AXISLOAD& out);
    bool getSpindleInfo(MTC_SPINDLE_INFO& out);
    bool getSpindleLoad(unsigned short path, unsigned short& out);
    bool getCurrentSpindleRev(unsigned short path, int& out);
    bool getOrderSpindleRev(unsigned short path, int& out);
    bool getSpindleOverRide(unsigned short path, unsigned short& out);
    bool getFeed(unsigned short path, MAZ_FEED& out);
    bool getFeedOverRide(unsigned short path, unsigned short& out);
    bool getRapidOverRide(unsigned short path, unsigned short& out);
    bool getCurrentTool(unsigned short path, MAZ_TOOLINFO& out);
    bool getToolOffsetComp(MAZ_TDCOMPALL& out);
    bool getAlarm(MAZ_ALARMALL& out);
    bool getMainPro(unsigned short path, MAZ_PROINFO& out);
    bool getRunningPro(unsigned short path, MAZ_PROINFO& out);
    bool getProgramInfo(MTC_PROGRAM_INFO& out);
    bool getAccumulatedTime(unsigned short path, MAZ_ACCUM_TIME& out);
    bool getNcPowerOnTime(MAZ_NCONTIME& out);
    bool getRunningTime(unsigned short path, MAZ_NCTIME& out);
    bool getPartsCount(unsigned short path, int& out);
    bool getMachineUnit(short& out);
    bool getClientVer(MAZ_VERINFO& out);
    bool getServerVer(MAZ_VERINFO& out);
    bool getNCVersionInfo(MAZ_NC_VERINFO& out);

    // === 参数/寄存器读取 ===
    bool getRangedParam(int start, int count, int* out);
    bool getRangedCmnVar(short type, short start, short count, double* out);
    bool getRangedRReg(int start, int count, int* out);
    bool getRangedWorkCoordSys(int start, int count, double* out);
    bool getRangedToolOffsetComp(int start, int count, MAZ_TOFFCOMP* out);

    // === 写入函数 ===
    bool setRangedParam(int start, int count, int* data);
    bool setRangedCmnVar(short type, short start, short count, double* data);
    bool setRangedRReg(int start, int count, int* data);
    bool setRangedWorkCoordSys(int start, int count, double* data);
    bool setRangedToolOffsetComp(int start, int count, MAZ_TOFFCOMP* data);
    bool reset();

    // 最后一次 DLL 错误
    int lastError() const { return m_lastError; }

private:
    MazakDLLWrapper() = default;
    ~MazakDLLWrapper();

    bool resolveAll();
    template<typename T> bool resolve(const char* name, T& func);

    QLibrary m_library;
    bool m_connected = false;
    unsigned short m_handle = 0;
    int m_lastError = 0;

    // 所有函数指针（与 typedef 一一对应）
    MazConnectFunc           m_connect = nullptr;
    MazDisconnectFunc        m_disconnect = nullptr;
    GetRunningInfoFunc       m_getRunningInfo = nullptr;
    GetRunningStsFunc        m_getRunningSts = nullptr;
    GetRunModeFunc           m_getRunMode = nullptr;
    GetAxisInfoFunc          m_getAxisInfo = nullptr;
    GetCurrentPosFunc        m_getCurrentPos = nullptr;
    GetRemainFunc            m_getRemain = nullptr;
    GetAxisNameFunc          m_getAxisName = nullptr;
    GetAxisLoadFunc          m_getAxisLoad = nullptr;
    GetSpindleInfoFunc       m_getSpindleInfo = nullptr;
    GetSpindleLoadFunc       m_getSpindleLoad = nullptr;
    GetCurrentSpindleRevFunc m_getCurrentSpindleRev = nullptr;
    GetOrderSpindleRevFunc   m_getOrderSpindleRev = nullptr;
    GetSpindleOverRideFunc   m_getSpindleOverRide = nullptr;
    GetFeedFunc              m_getFeed = nullptr;
    GetFeedOverRideFunc      m_getFeedOverRide = nullptr;
    GetRapidOverRideFunc     m_getRapidOverRide = nullptr;
    GetCurrentToolFunc       m_getCurrentTool = nullptr;
    GetToolOffsetCompFunc    m_getToolOffsetComp = nullptr;
    GetAlarmFunc             m_getAlarm = nullptr;
    GetMainProFunc           m_getMainPro = nullptr;
    GetRunningProFunc        m_getRunningPro = nullptr;
    GetProgramInfoFunc       m_getProgramInfo = nullptr;
    GetAccumulatedTimeFunc   m_getAccumulatedTime = nullptr;
    GetNcPowerOnTimeFunc     m_getNcPowerOnTime = nullptr;
    GetRunningTimeFunc       m_getRunningTime = nullptr;
    GetPreparationTimeFunc   m_getPreparationTime = nullptr;
    GetPartsCountFunc        m_getPartsCount = nullptr;
    GetMachineUnitFunc       m_getMachineUnit = nullptr;
    GetClientVerFunc         m_getClientVer = nullptr;
    GetServerVerFunc         m_getServerVer = nullptr;
    GetNCVersionInfoFunc     m_getNCVersionInfo = nullptr;
    GetRangedParamFunc       m_getRangedParam = nullptr;
    SetRangedParamFunc       m_setRangedParam = nullptr;
    GetRangedCmnVarFunc      m_getRangedCmnVar = nullptr;
    SetRangedCmnVarFunc      m_setRangedCmnVar = nullptr;
    GetRangedRRegFunc        m_getRangedRReg = nullptr;
    SetRangedRRegFunc        m_setRangedRReg = nullptr;
    GetRangedWorkCoordSysFunc m_getRangedWorkCoordSys = nullptr;
    SetRangedWorkCoordSysFunc m_setRangedWorkCoordSys = nullptr;
    GetRangedToolOffsetCompFunc m_getRangedToolOffsetComp = nullptr;
    SetRangedToolOffsetCompFunc m_setRangedToolOffsetComp = nullptr;
    MazResetFunc             m_reset = nullptr;
};
```

- [ ] **Step 2: 创建 mazakdllwrapper.cpp**

实现 load/unload、connect/disconnect、所有读取和写入方法。每个方法模式相同：检查函数指针 → 调用 → 记录错误码 → 返回 bool。

模板 resolve 方法：
```cpp
template<typename T>
bool MazakDLLWrapper::resolve(const char* name, T& func)
{
    func = reinterpret_cast<T>(m_library.resolve(name));
    if (!func) qWarning() << "MazakDLL: failed to resolve" << name;
    return func != nullptr;
}
```

load 方法调用 resolveAll() 解析所有函数指针。connect 调用 MazConnect 并存储句柄。每个 getXxx 方法检查指针非空后调用 DLL，记录 m_lastError。

getRangedParam/setRangedParam 等参数函数直接透传 DLL 的 start/count/数组参数。

- [ ] **Step 3: 编译验证**

- [ ] **Step 4: Commit**

```bash
git add src/protocol/mazak/mazakdllwrapper.h src/protocol/mazak/mazakdllwrapper.cpp
git commit -m "feat(mazak): add DLL wrapper with QLibrary loading for NTIFDLL.dll"
```

---

### Task 3: MazakProtocol（IProtocol 适配）

**Files:**
- Create: `src/protocol/mazak/mazakprotocol.h`
- Create: `src/protocol/mazak/mazakprotocol.cpp`

实现 IProtocol 接口。由于 Mazak 不走帧级通信，IProtocol 方法做适配：
- `buildRequest()` / `parseResponse()` 不实际使用
- `extractFrame()` 直接返回 buffer
- `allCommands()` / `commandDef()` 返回 DLL 功能的命令列表
- `interpretData()` 格式化 DLL 数据

**命令编码：**

| cmdCode | 名称 | DLL 函数 |
|---------|------|---------|
| 0x01 | 运行信息 | getRunningInfo |
| 0x02 | 运行状态 | getRunningSts |
| 0x03 | 运行模式 | getRunMode |
| 0x10 | 轴信息 | getAxisInfo |
| 0x11 | 当前坐标 | getCurrentPos |
| 0x12 | 剩余距离 | getRemain |
| 0x13 | 轴名 | getAxisName |
| 0x20 | 主轴信息 | getSpindleInfo |
| 0x21 | 主轴负载 | getSpindleLoad |
| 0x22 | 实际转速 | getCurrentSpindleRev |
| 0x23 | 设定转速 | getOrderSpindleRev |
| 0x30 | 进给速度 | getFeed |
| 0x40 | 当前刀具 | getCurrentTool |
| 0x41 | 刀补数据 | getToolOffsetComp |
| 0x50 | 报警 | getAlarm |
| 0x60 | 主程序 | getMainPro |
| 0x61 | 运行程序 | getRunningPro |
| 0x70 | NC 参数读写 | getRangedParam / setRangedParam |
| 0x71 | 宏变量读写 | getRangedCmnVar / setRangedCmnVar |
| 0x72 | R 寄存器读写 | getRangedRReg / setRangedRReg |
| 0x73 | 工件坐标系读写 | getRangedWorkCoordSys / setRangedWorkCoordSys |
| 0x74 | 刀补读写 | getRangedToolOffsetComp / setRangedToolOffsetComp |
| 0xA0 | 累计时间 | getAccumulatedTime |
| 0xA1 | 开机时间 | getNcPowerOnTime |
| 0xA2 | 运行时间 | getRunningTime |
| 0xF0 | NC 复位 | reset |

MazakProtocol 的 `name()` 返回 "Mazak Smooth"，`version()` 返回 "1.0"。

- [ ] **Step 1: 创建头文件和源文件，实现 IProtocol 所有纯虚方法**

- [ ] **Step 2: 编译验证**

- [ ] **Step 3: Commit**

```bash
git add src/protocol/mazak/mazakprotocol.h src/protocol/mazak/mazakprotocol.cpp
git commit -m "feat(mazak): add IProtocol adapter with DLL command definitions"
```

---

### Task 4: Widget 工厂

**Files:**
- Create: `src/protocol/mazak/mazakwidgetfactory.h`
- Create: `src/protocol/mazak/mazakwidgetfactory.cpp`

遵循现有模式，和 SiemensWidgetFactory 结构一致。

```cpp
// mazakwidgetfactory.h
#ifndef MAZAKWIDGETFACTORY_H
#define MAZAKWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class MazakWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif
```

```cpp
// mazakwidgetfactory.cpp
#include "protocol/mazak/mazakwidgetfactory.h"
#include "protocol/mazak/mazakrealtimewidget.h"
#include "protocol/mazak/mazakcommandwidget.h"
#include "protocol/mazak/mazakparsewidget.h"

QWidget* MazakWidgetFactory::createRealtimeWidget() {
    return new MazakRealtimeWidget;
}
QWidget* MazakWidgetFactory::createCommandWidget() {
    return new MazakCommandWidget;
}
QWidget* MazakWidgetFactory::createParseWidget() {
    return new MazakParseWidget;
}
```

- [ ] **Step 1: 创建工厂文件**

- [ ] **Step 2: Commit**

```bash
git add src/protocol/mazak/mazakwidgetfactory.h src/protocol/mazak/mazakwidgetfactory.cpp
git commit -m "feat(mazak): add widget factory"
```

---

### Task 5: RealtimeWidget — 实时监控

**Files:**
- Create: `src/protocol/mazak/mazakrealtimewidget.h`
- Create: `src/protocol/mazak/mazakrealtimewidget.cpp`

**关键区别：** 与其他协议不同，Mazak RealtimeWidget 直接调用 DLL，不通过 MainWindow 的 pollRequest 信号。内部 QTimer 驱动轮询。

**布局（参照 Fanuc/Siemens 的 3+1 行）：**

```
┌─────────────────────────────────────────────────┐
│ [状态] 运行  [模式] 自动  [程序] O1234  [件数] 5 │  ← 第1行: 系统状态
├─────────────────────────────────────────────────┤
│ [设定S] 3000 [实际S] 2980 [S负载] 45% [S倍率] 100%│  ← 第2行: 主轴+进给
│ [进给] 1200  [F倍率] 100%  [快移%] 100%          │
├─────────────────────────────────────────────────┤
│ 轴  │ 机械坐标    │ 绝对坐标    │ 剩余距离      │  ← 第3行: 坐标
│ X   │ 100.000     │ 50.000      │ 10.000        │
│ Y   │ 200.000     │ 80.000      │ 5.000         │
│ Z   │ 150.000     │ 30.000      │ 2.000         │
├─────────────────────────────────────────────────┤
│ [报警] 无  [刀号] T05  [开机] 12:30:00 [运行] 05:20│  ← 底部信息栏
└─────────────────────────────────────────────────┘
```

**轮询逻辑：**

```cpp
void MazakRealtimeWidget::onPollTimeout()
{
    auto& dll = MazakDLLWrapper::instance();
    if (!dll.isConnected()) { stopPolling(); return; }

    MTC_RUNNING_INFO ri;
    if (dll.getRunningInfo(ri)) {
        // 更新状态、模式、刀号、倍率、件数、报警号
        updateStatusLabel(ri.run_info[0]);
    }

    MTC_AXIS_INFO ai;
    if (dll.getAxisInfo(ai)) {
        // 更新坐标表格
        updateAxisTable(ai);
    }

    MTC_SPINDLE_INFO si;
    if (dll.getSpindleInfo(si)) {
        // 更新主轴转速、负载、温度
        updateSpindleLabels(si);
    }

    MAZ_ALARMALL al;
    if (dll.getAlarm(al)) {
        // 更新报警显示
        updateAlarmLabel(al);
    }

    MAZ_ACCUM_TIME at;
    if (dll.getAccumulatedTime(0, at)) {
        // 更新时间
        updateTimeLabels(at);
    }
}
```

**状态值映射（从 C# MazakTCPLib.cs 移植）：**

运行状态判断逻辑：
- ncsts 在 {4111,15,1807,1551,1039,1295,1815,1443,271} → 运行
- ncsts 在 {5647,5903,1955,259,1283,1795,1827,5891} → 待机
- almno 非零且不是 {1370,0,468,72,82} → 报警
- 其他 → 离线

模式映射：
- 1=快速, 2=手动(MF2), 16=回零, 256=自动(Memory), 512=Tape, 2048=MDI

- [ ] **Step 1: 创建头文件，声明 MazakRealtimeWidget 类**

继承 QWidget，包含：
- QLabel 成员用于状态/模式/程序名/件数/主轴/进给/报警/时间
- QTableWidget 用于坐标显示
- QTimer 用于轮询
- startPolling()/stopPolling()/clearData() 公共方法

- [ ] **Step 2: 创建源文件，实现布局和轮询逻辑**

- [ ] **Step 3: 编译验证**

- [ ] **Step 4: Commit**

```bash
git add src/protocol/mazak/mazakrealtimewidget.h src/protocol/mazak/mazakrealtimewidget.cpp
git commit -m "feat(mazak): add realtime monitoring widget with DLL polling"
```

---

### Task 6: CommandWidget — 命令下发

**Files:**
- Create: `src/protocol/mazak/mazakcommandwidget.h`
- Create: `src/protocol/mazak/mazakcommandwidget.cpp`

**布局：** QTabWidget 包含多个 Tab 页，每个 Tab 对应一类参数读写。

**Tab 页列表：**
1. **NC 参数** — 起始号 + 数量 + 读取/写入按钮 + 值表格
2. **宏变量** — 类型选择 + 起始号 + 数量 + 读取/写入
3. **R 寄存器** — 起始号 + 数量 + 读取/写入
4. **工件坐标系** — 起始号 + 数量 + 读取/写入
5. **刀补** — 起始号 + 数量 + 读取/写入
6. **操作** — NC 复位按钮

每个参数 Tab 的通用交互模式：
1. 用户输入起始编号和数量
2. 点"读取" → 调用对应的 DLL getRangedXxx → 填充 QTableWidget
3. 用户可编辑表格中的值
4. 点"写入" → 从表格读取值 → 调用对应的 DLL setRangedXxx
5. 状态栏显示操作结果（成功/错误码）

- [ ] **Step 1: 创建头文件和源文件，实现 Tab 页布局和读写逻辑**

- [ ] **Step 2: 编译验证**

- [ ] **Step 3: Commit**

```bash
git add src/protocol/mazak/mazakcommandwidget.h src/protocol/mazak/mazakcommandwidget.cpp
git commit -m "feat(mazak): add command widget with parameter/tool/coord read/write tabs"
```

---

### Task 7: ParseWidget — 数据解析

**Files:**
- Create: `src/protocol/mazak/mazakparsewidget.h`
- Create: `src/protocol/mazak/mazakparsewidget.cpp`

简单工具页：
- QComboBox 选择数据类型（运行信息、轴信息、主轴信息、报警等）
- QTextEdit 用于十六进制输入
- "解析"按钮
- QTextEdit 显示解析结果

将输入的 QByteArray 按 `reinterpret_cast` 转换为对应结构体，逐字段显示。

- [ ] **Step 1: 创建头文件和源文件**

- [ ] **Step 2: 编译验证**

- [ ] **Step 3: Commit**

```bash
git add src/protocol/mazak/mazakparsewidget.h src/protocol/mazak/mazakparsewidget.cpp
git commit -m "feat(mazak): add data parse widget"
```

---

### Task 8: MainWindow 集成 + 构建系统

**Files:**
- Modify: `GSK988Tool.pro`
- Modify: `src/mainwindow.h`
- Modify: `src/mainwindow.cpp`

**改动要点：**

1. **GSK988Tool.pro** — 添加 12 个新文件到 SOURCES 和 HEADERS
2. **mainwindow.h** — 前向声明 `class MazakRealtimeWidget; class MazakCommandWidget;`
3. **mainwindow.cpp** — 三处改动：

   a) `setupToolBar()` — 添加 `"Mazak Smooth"` 到协议下拉框

   b) `switchProtocol()` — 添加 case 4:
   ```cpp
   case 4:
       m_protocol = new MazakProtocol(this);
       m_widgetFactory = new MazakWidgetFactory;
       break;
   ```

   c) `onConnectClicked()` — Mazak 特殊连接处理：
   - 检查当前协议是否为 Mazak（index == 4）
   - 如果是：从 Transport 配置获取 IP/端口 → load DLL → DLL connect
   - 跳过 Transport 层连接
   - 连接成功后启动 RealtimeWidget 轮询

   d) `switchProtocol()` 中停止旧轮询 — 添加 MazakRealtimeWidget 的 stopPolling 分支

   e) `onConnectClicked()` 断开时 — 添加 Mazak 断开处理（DLL disconnect + unload）

   f) Transport 断开信号中 — 添加 MazakRealtimeWidget 的 stopPolling/clearData

- [ ] **Step 1: 修改 GSK988Tool.pro，添加所有新文件**

- [ ] **Step 2: 修改 mainwindow.h，添加前向声明**

- [ ] **Step 3: 修改 mainwindow.cpp，添加 Mazak 协议注册和连接处理**

- [ ] **Step 4: 编译验证 — 确保所有协议都能正常切换**

- [ ] **Step 5: Commit**

```bash
git add GSK988Tool.pro src/mainwindow.h src/mainwindow.cpp
git commit -m "feat(mazak): integrate Mazak Smooth protocol into MainWindow and build system"
```

---

### Task 9: DLL 分发和最终验证

**Files:**
- Copy: `NTIFDLL.dll` → 构建输出目录

- [ ] **Step 1: 将 NTIFDLL.dll 复制到构建输出目录**

从 `C:\Users\QC\Desktop\mazaksmooth\mazaksmooth\bin\Debug\NTIFDLL.dll` 复制到程序的构建输出目录。

- [ ] **Step 2: 在 .pro 中添加 DLL 复制命令（可选）**

或者手动复制，确保程序运行时能找到 DLL。

- [ ] **Step 3: 运行程序，切换到 Mazak Smooth 协议**

验证：
- 协议下拉框显示 "Mazak Smooth"
- 三件套 Tab 正常显示
- 切换协议不崩溃
- 连接面板显示 IP/端口（即使不用 Transport 层，也用配置面板输入参数）

- [ ] **Step 4: 最终 Commit**

```bash
git add -A
git commit -m "feat(mazak): complete Mazak Smooth protocol integration"
```
