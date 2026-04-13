#ifndef MAZAKDLLWRAPPER_H
#define MAZAKDLLWRAPPER_H

#include <QLibrary>
#include <QString>
#include <QDebug>
#include "protocol/mazak/mazakframebuilder.h"

class MazakDLLWrapper
{
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
    void setHandle(unsigned short h) { m_handle = h; m_connected = (h != 0); }
    void clearConnection() { m_handle = 0; m_connected = false; }

    // 运行状态
    bool getRunningInfo(MTC_RUNNING_INFO& out);
    bool getRunningSts(unsigned short path, short& out);
    bool getRunMode(unsigned short path, short& out);

    // 坐标
    bool getAxisInfo(MTC_AXIS_INFO& out);
    bool getCurrentPos(MAZ_NCPOS& out);
    bool getRemain(MAZ_NCPOS& out);
    bool getAxisName(MAZ_AXISNAME& out);
    bool getAxisLoad(MAZ_AXISLOAD& out);

    // 主轴
    bool getSpindleInfo(MTC_SPINDLE_INFO& out);
    bool getSpindleLoad(unsigned short path, unsigned short& out);
    bool getCurrentSpindleRev(unsigned short path, int& out);
    bool getOrderSpindleRev(unsigned short path, int& out);
    bool getSpindleOverRide(unsigned short path, unsigned short& out);

    // 进给
    bool getFeed(unsigned short path, MAZ_FEED& out);
    bool getFeedOverRide(unsigned short path, unsigned short& out);
    bool getRapidOverRide(unsigned short path, unsigned short& out);

    // 刀具
    bool getCurrentTool(unsigned short path, MAZ_TOOLINFO& out);
    bool getToolOffsetComp(MAZ_TDCOMPALL& out);

    // 报警
    bool getAlarm(MAZ_ALARMALL& out);

    // 程序
    bool getMainPro(unsigned short path, MAZ_PROINFO& out);
    bool getRunningPro(unsigned short path, MAZ_PROINFO& out);
    bool getProgramInfo(MTC_PROGRAM_INFO& out);

    // 时间
    bool getAccumulatedTime(unsigned short path, MAZ_ACCUM_TIME& out);
    bool getNcPowerOnTime(MAZ_NCONTIME& out);
    bool getRunningTime(unsigned short path, MAZ_NCTIME& out);
    bool getPreparationTime(unsigned short path, MAZ_NCTIME& out);

    // 系统
    bool getPartsCount(unsigned short path, int& out);
    bool getMachineUnit(short& out);
    bool getClientVer(MAZ_VERINFO& out);
    bool getServerVer(MAZ_VERINFO& out);
    bool getNCVersionInfo(MAZ_NC_VERINFO& out);

    // 参数/寄存器读取
    bool getRangedParam(int start, int count, int* out);
    bool getRangedCmnVar(short type, short start, short count, double* out);
    bool getRangedRReg(int start, int count, int* out);
    bool getRangedWorkCoordSys(int start, int count, double* out);
    bool getRangedToolOffsetComp(int start, int count, MAZ_TOFFCOMP* out);

    // 写入
    bool setRangedParam(int start, int count, int* data);
    bool setRangedCmnVar(short type, short start, short count, double* data);
    bool setRangedRReg(int start, int count, int* data);
    bool setRangedWorkCoordSys(int start, int count, double* data);
    bool setRangedToolOffsetComp(int start, int count, MAZ_TOFFCOMP* data);
    bool reset();

    int lastError() const { return m_lastError; }

private:
    MazakDLLWrapper() = default;
    ~MazakDLLWrapper();
    Q_DISABLE_COPY(MazakDLLWrapper)

    bool resolveAll();

    QLibrary m_library;
    bool m_connected = false;
    unsigned short m_handle = 0;
    int m_lastError = 0;

    // 函数指针
    using MazConnectFunc                = int(*)(unsigned short*, const char*, int, int);
    using MazDisconnectFunc             = int(*)(unsigned short);
    using GetRunningInfoFunc            = int(*)(unsigned short, MTC_RUNNING_INFO*);
    using GetRunningStsFunc             = int(*)(unsigned short, unsigned short, short*);
    using GetRunModeFunc                = int(*)(unsigned short, unsigned short, short*);
    using GetAxisInfoFunc               = int(*)(unsigned short, MTC_AXIS_INFO*);
    using GetCurrentPosFunc             = int(*)(unsigned short, MAZ_NCPOS*);
    using GetRemainFunc                 = int(*)(unsigned short, MAZ_NCPOS*);
    using GetAxisNameFunc               = int(*)(unsigned short, MAZ_AXISNAME*);
    using GetAxisLoadFunc               = int(*)(unsigned short, MAZ_AXISLOAD*);
    using GetSpindleInfoFunc            = int(*)(unsigned short, MTC_SPINDLE_INFO*);
    using GetSpindleLoadFunc            = int(*)(unsigned short, unsigned short, unsigned short*);
    using GetCurrentSpindleRevFunc      = int(*)(unsigned short, unsigned short, int*);
    using GetOrderSpindleRevFunc        = int(*)(unsigned short, unsigned short, int*);
    using GetSpindleOverRideFunc        = int(*)(unsigned short, unsigned short, unsigned short*);
    using GetFeedFunc                   = int(*)(unsigned short, unsigned short, MAZ_FEED*);
    using GetFeedOverRideFunc           = int(*)(unsigned short, unsigned short, unsigned short*);
    using GetRapidOverRideFunc          = int(*)(unsigned short, unsigned short, unsigned short*);
    using GetCurrentToolFunc            = int(*)(unsigned short, unsigned short, MAZ_TOOLINFO*);
    using GetToolOffsetCompFunc         = int(*)(unsigned short, MAZ_TDCOMPALL*);
    using GetAlarmFunc                  = int(*)(unsigned short, MAZ_ALARMALL*);
    using GetMainProFunc                = int(*)(unsigned short, unsigned short, MAZ_PROINFO*);
    using GetRunningProFunc             = int(*)(unsigned short, unsigned short, MAZ_PROINFO*);
    using GetProgramInfoFunc            = int(*)(unsigned short, MTC_PROGRAM_INFO*);
    using GetAccumulatedTimeFunc        = int(*)(unsigned short, unsigned short, MAZ_ACCUM_TIME*);
    using GetNcPowerOnTimeFunc          = int(*)(unsigned short, MAZ_NCONTIME*);
    using GetRunningTimeFunc            = int(*)(unsigned short, unsigned short, MAZ_NCTIME*);
    using GetPreparationTimeFunc        = int(*)(unsigned short, unsigned short, MAZ_NCTIME*);
    using GetPartsCountFunc             = int(*)(unsigned short, unsigned short, int*);
    using GetMachineUnitFunc            = int(*)(unsigned short, short*);
    using GetClientVerFunc              = int(*)(unsigned short, MAZ_VERINFO*);
    using GetServerVerFunc              = int(*)(unsigned short, MAZ_VERINFO*);
    using GetNCVersionInfoFunc          = int(*)(unsigned short, MAZ_NC_VERINFO*);
    using GetRangedParamFunc            = int(*)(unsigned short, int, int, int*);
    using SetRangedParamFunc            = int(*)(unsigned short, int, int, int*);
    using GetRangedCmnVarFunc           = int(*)(unsigned short, short, short, short, double*);
    using SetRangedCmnVarFunc           = int(*)(unsigned short, short, short, short, double*);
    using GetRangedRRegFunc             = int(*)(unsigned short, int, int, int*);
    using SetRangedRRegFunc             = int(*)(unsigned short, int, int, int*);
    using GetRangedWorkCoordSysFunc     = int(*)(unsigned short, int, int, double*);
    using SetRangedWorkCoordSysFunc     = int(*)(unsigned short, int, int, double*);
    using GetRangedToolOffsetCompFunc   = int(*)(unsigned short, int, int, MAZ_TOFFCOMP*);
    using SetRangedToolOffsetCompFunc   = int(*)(unsigned short, int, int, MAZ_TOFFCOMP*);
    using MazResetFunc                  = int(*)(unsigned short);

    MazConnectFunc                m_connect = nullptr;
    MazDisconnectFunc             m_disconnect = nullptr;
    GetRunningInfoFunc            m_getRunningInfo = nullptr;
    GetRunningStsFunc             m_getRunningSts = nullptr;
    GetRunModeFunc                m_getRunMode = nullptr;
    GetAxisInfoFunc               m_getAxisInfo = nullptr;
    GetCurrentPosFunc             m_getCurrentPos = nullptr;
    GetRemainFunc                 m_getRemain = nullptr;
    GetAxisNameFunc               m_getAxisName = nullptr;
    GetAxisLoadFunc               m_getAxisLoad = nullptr;
    GetSpindleInfoFunc            m_getSpindleInfo = nullptr;
    GetSpindleLoadFunc            m_getSpindleLoad = nullptr;
    GetCurrentSpindleRevFunc      m_getCurrentSpindleRev = nullptr;
    GetOrderSpindleRevFunc        m_getOrderSpindleRev = nullptr;
    GetSpindleOverRideFunc        m_getSpindleOverRide = nullptr;
    GetFeedFunc                   m_getFeed = nullptr;
    GetFeedOverRideFunc           m_getFeedOverRide = nullptr;
    GetRapidOverRideFunc          m_getRapidOverRide = nullptr;
    GetCurrentToolFunc            m_getCurrentTool = nullptr;
    GetToolOffsetCompFunc         m_getToolOffsetComp = nullptr;
    GetAlarmFunc                  m_getAlarm = nullptr;
    GetMainProFunc                m_getMainPro = nullptr;
    GetRunningProFunc             m_getRunningPro = nullptr;
    GetProgramInfoFunc            m_getProgramInfo = nullptr;
    GetAccumulatedTimeFunc        m_getAccumulatedTime = nullptr;
    GetNcPowerOnTimeFunc          m_getNcPowerOnTime = nullptr;
    GetRunningTimeFunc            m_getRunningTime = nullptr;
    GetPreparationTimeFunc        m_getPreparationTime = nullptr;
    GetPartsCountFunc             m_getPartsCount = nullptr;
    GetMachineUnitFunc            m_getMachineUnit = nullptr;
    GetClientVerFunc              m_getClientVer = nullptr;
    GetServerVerFunc              m_getServerVer = nullptr;
    GetNCVersionInfoFunc          m_getNCVersionInfo = nullptr;
    GetRangedParamFunc            m_getRangedParam = nullptr;
    SetRangedParamFunc            m_setRangedParam = nullptr;
    GetRangedCmnVarFunc           m_getRangedCmnVar = nullptr;
    SetRangedCmnVarFunc           m_setRangedCmnVar = nullptr;
    GetRangedRRegFunc             m_getRangedRReg = nullptr;
    SetRangedRRegFunc             m_setRangedRReg = nullptr;
    GetRangedWorkCoordSysFunc     m_getRangedWorkCoordSys = nullptr;
    SetRangedWorkCoordSysFunc     m_setRangedWorkCoordSys = nullptr;
    GetRangedToolOffsetCompFunc   m_getRangedToolOffsetComp = nullptr;
    SetRangedToolOffsetCompFunc   m_setRangedToolOffsetComp = nullptr;
    MazResetFunc                  m_reset = nullptr;
};

#endif // MAZAKDLLWRAPPER_H
