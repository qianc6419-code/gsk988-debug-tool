#include "protocol/mazak/mazakdllwrapper.h"

MazakDLLWrapper::~MazakDLLWrapper()
{
    unload();
}

MazakDLLWrapper& MazakDLLWrapper::instance()
{
    static MazakDLLWrapper inst;
    return inst;
}

bool MazakDLLWrapper::load(const QString& dllPath)
{
    if (m_library.isLoaded()) return true;
    m_library.setFileName(dllPath);
    if (!m_library.load()) {
        qWarning() << "MazakDLL: failed to load" << dllPath << m_library.errorString();
        return false;
    }
    return resolveAll();
}

bool MazakDLLWrapper::isLoaded() const
{
    return m_library.isLoaded();
}

void MazakDLLWrapper::unload()
{
    if (m_connected && m_disconnect && m_handle) {
        m_disconnect(m_handle);
    }
    m_connected = false;
    m_handle = 0;
    if (m_library.isLoaded()) m_library.unload();
}

bool MazakDLLWrapper::resolveAll()
{
    bool ok = true;
    auto resolve = [&](const char* name, auto& func) {
        func = reinterpret_cast<decltype(func)>(m_library.resolve(name));
        if (!func) {
            qWarning() << "MazakDLL: failed to resolve" << name;
            ok = false;
        }
    };

    resolve("MazConnect", m_connect);
    resolve("MazDisconnect", m_disconnect);
    resolve("MazGetRunningInfo", m_getRunningInfo);
    resolve("MazGetRunningSts", m_getRunningSts);
    resolve("MazGetRunMode", m_getRunMode);
    resolve("MazGetAxisInfo", m_getAxisInfo);
    resolve("MazGetCurrentPos", m_getCurrentPos);
    resolve("MazGetRemain", m_getRemain);
    resolve("MazGetAxisName", m_getAxisName);
    resolve("MazGetAxisLoad", m_getAxisLoad);
    resolve("MazGetSpindleInfo", m_getSpindleInfo);
    resolve("MazGetSpindleLoad", m_getSpindleLoad);
    resolve("MazGetCurrentSpindleRev", m_getCurrentSpindleRev);
    resolve("MazGetOrderSpindleRev", m_getOrderSpindleRev);
    resolve("MazGetSpindleOverRide", m_getSpindleOverRide);
    resolve("MazGetFeed", m_getFeed);
    resolve("MazGetFeedOverRide", m_getFeedOverRide);
    resolve("MazGetRapidOverRide", m_getRapidOverRide);
    resolve("MazGetCurrentTool", m_getCurrentTool);
    resolve("MazGetToolOffsetComp", m_getToolOffsetComp);
    resolve("MazGetAlarm", m_getAlarm);
    resolve("MazGetMainPro", m_getMainPro);
    resolve("MazGetRunningPro", m_getRunningPro);
    resolve("MazGetProgramInfo", m_getProgramInfo);
    resolve("MazGetAccumulatedTime", m_getAccumulatedTime);
    resolve("MazGetNcPowerOnTime", m_getNcPowerOnTime);
    resolve("MazGetRunningTime", m_getRunningTime);
    resolve("MazGetPreparationTime", m_getPreparationTime);
    resolve("MazGetPartsCount", m_getPartsCount);
    resolve("MazGetMachineUnit", m_getMachineUnit);
    resolve("MazGetClientVer", m_getClientVer);
    resolve("MazGetServerVer", m_getServerVer);
    resolve("MazGetNCVersionInfo", m_getNCVersionInfo);
    resolve("MazGetRangedParam", m_getRangedParam);
    resolve("MazSetRangedParam", m_setRangedParam);
    resolve("MazGetRangedCmnVar", m_getRangedCmnVar);
    resolve("MazSetRangedCmnVar", m_setRangedCmnVar);
    resolve("MazGetRangedRRegister", m_getRangedRReg);
    resolve("MazSetRangedRRegister", m_setRangedRReg);
    resolve("MazGetRangedWorkCoordSys", m_getRangedWorkCoordSys);
    resolve("MazSetRangedWorkCoordSys", m_setRangedWorkCoordSys);
    resolve("MazGetRangedToolOffsetComp", m_getRangedToolOffsetComp);
    resolve("MazSetRangedToolOffsetComp", m_setRangedToolOffsetComp);
    resolve("MazReset", m_reset);

    return ok;
}

// === 连接管理 ===

bool MazakDLLWrapper::connect(unsigned short& handle, const QString& ip, int port, int timeout)
{
    if (!m_connect) return false;
    QByteArray ipBuf = ip.toLocal8Bit();
    int ret = m_connect(&handle, ipBuf.constData(), port, timeout);
    m_lastError = ret;
    if (ret == MazakError::OK) {
        m_handle = handle;
        m_connected = true;
        return true;
    }
    return false;
}

bool MazakDLLWrapper::disconnect(unsigned short handle)
{
    if (!m_disconnect) return false;
    int ret = m_disconnect(handle);
    m_lastError = ret;
    m_connected = false;
    m_handle = 0;
    return (ret == MazakError::OK);
}

// === 读取函数 ===

#define MAZAK_CALL(func, ...) \
    if (!func) return false; \
    int ret = func(__VA_ARGS__); \
    m_lastError = ret; \
    return (ret == MazakError::OK)

bool MazakDLLWrapper::getRunningInfo(MTC_RUNNING_INFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getRunningInfo, m_handle, &out);
}

bool MazakDLLWrapper::getRunningSts(unsigned short path, short& out)
{
    out = 0;
    MAZAK_CALL(m_getRunningSts, m_handle, path, &out);
}

bool MazakDLLWrapper::getRunMode(unsigned short path, short& out)
{
    out = 0;
    MAZAK_CALL(m_getRunMode, m_handle, path, &out);
}

bool MazakDLLWrapper::getAxisInfo(MTC_AXIS_INFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getAxisInfo, m_handle, &out);
}

bool MazakDLLWrapper::getCurrentPos(MAZ_NCPOS& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getCurrentPos, m_handle, &out);
}

bool MazakDLLWrapper::getRemain(MAZ_NCPOS& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getRemain, m_handle, &out);
}

bool MazakDLLWrapper::getAxisName(MAZ_AXISNAME& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getAxisName, m_handle, &out);
}

bool MazakDLLWrapper::getAxisLoad(MAZ_AXISLOAD& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getAxisLoad, m_handle, &out);
}

bool MazakDLLWrapper::getSpindleInfo(MTC_SPINDLE_INFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getSpindleInfo, m_handle, &out);
}

bool MazakDLLWrapper::getSpindleLoad(unsigned short path, unsigned short& out)
{
    out = 0;
    MAZAK_CALL(m_getSpindleLoad, m_handle, path, &out);
}

bool MazakDLLWrapper::getCurrentSpindleRev(unsigned short path, int& out)
{
    out = 0;
    MAZAK_CALL(m_getCurrentSpindleRev, m_handle, path, &out);
}

bool MazakDLLWrapper::getOrderSpindleRev(unsigned short path, int& out)
{
    out = 0;
    MAZAK_CALL(m_getOrderSpindleRev, m_handle, path, &out);
}

bool MazakDLLWrapper::getSpindleOverRide(unsigned short path, unsigned short& out)
{
    out = 0;
    MAZAK_CALL(m_getSpindleOverRide, m_handle, path, &out);
}

bool MazakDLLWrapper::getFeed(unsigned short path, MAZ_FEED& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getFeed, m_handle, path, &out);
}

bool MazakDLLWrapper::getFeedOverRide(unsigned short path, unsigned short& out)
{
    out = 0;
    MAZAK_CALL(m_getFeedOverRide, m_handle, path, &out);
}

bool MazakDLLWrapper::getRapidOverRide(unsigned short path, unsigned short& out)
{
    out = 0;
    MAZAK_CALL(m_getRapidOverRide, m_handle, path, &out);
}

bool MazakDLLWrapper::getCurrentTool(unsigned short path, MAZ_TOOLINFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getCurrentTool, m_handle, path, &out);
}

bool MazakDLLWrapper::getToolOffsetComp(MAZ_TDCOMPALL& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getToolOffsetComp, m_handle, &out);
}

bool MazakDLLWrapper::getAlarm(MAZ_ALARMALL& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getAlarm, m_handle, &out);
}

bool MazakDLLWrapper::getMainPro(unsigned short path, MAZ_PROINFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getMainPro, m_handle, path, &out);
}

bool MazakDLLWrapper::getRunningPro(unsigned short path, MAZ_PROINFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getRunningPro, m_handle, path, &out);
}

bool MazakDLLWrapper::getProgramInfo(MTC_PROGRAM_INFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getProgramInfo, m_handle, &out);
}

bool MazakDLLWrapper::getAccumulatedTime(unsigned short path, MAZ_ACCUM_TIME& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getAccumulatedTime, m_handle, path, &out);
}

bool MazakDLLWrapper::getNcPowerOnTime(MAZ_NCONTIME& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getNcPowerOnTime, m_handle, &out);
}

bool MazakDLLWrapper::getRunningTime(unsigned short path, MAZ_NCTIME& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getRunningTime, m_handle, path, &out);
}

bool MazakDLLWrapper::getPreparationTime(unsigned short path, MAZ_NCTIME& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getPreparationTime, m_handle, path, &out);
}

bool MazakDLLWrapper::getPartsCount(unsigned short path, int& out)
{
    out = 0;
    MAZAK_CALL(m_getPartsCount, m_handle, path, &out);
}

bool MazakDLLWrapper::getMachineUnit(short& out)
{
    out = 0;
    MAZAK_CALL(m_getMachineUnit, m_handle, &out);
}

bool MazakDLLWrapper::getClientVer(MAZ_VERINFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getClientVer, m_handle, &out);
}

bool MazakDLLWrapper::getServerVer(MAZ_VERINFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getServerVer, m_handle, &out);
}

bool MazakDLLWrapper::getNCVersionInfo(MAZ_NC_VERINFO& out)
{
    memset(&out, 0, sizeof(out));
    MAZAK_CALL(m_getNCVersionInfo, m_handle, &out);
}

// === 参数/寄存器读取 ===

bool MazakDLLWrapper::getRangedParam(int start, int count, int* out)
{
    MAZAK_CALL(m_getRangedParam, m_handle, start, count, out);
}

bool MazakDLLWrapper::getRangedCmnVar(short type, short start, short count, double* out)
{
    MAZAK_CALL(m_getRangedCmnVar, m_handle, type, start, count, out);
}

bool MazakDLLWrapper::getRangedRReg(int start, int count, int* out)
{
    MAZAK_CALL(m_getRangedRReg, m_handle, start, count, out);
}

bool MazakDLLWrapper::getRangedWorkCoordSys(int start, int count, double* out)
{
    MAZAK_CALL(m_getRangedWorkCoordSys, m_handle, start, count, out);
}

bool MazakDLLWrapper::getRangedToolOffsetComp(int start, int count, MAZ_TOFFCOMP* out)
{
    MAZAK_CALL(m_getRangedToolOffsetComp, m_handle, start, count, out);
}

// === 写入函数 ===

bool MazakDLLWrapper::setRangedParam(int start, int count, int* data)
{
    MAZAK_CALL(m_setRangedParam, m_handle, start, count, data);
}

bool MazakDLLWrapper::setRangedCmnVar(short type, short start, short count, double* data)
{
    MAZAK_CALL(m_setRangedCmnVar, m_handle, type, start, count, data);
}

bool MazakDLLWrapper::setRangedRReg(int start, int count, int* data)
{
    MAZAK_CALL(m_setRangedRReg, m_handle, start, count, data);
}

bool MazakDLLWrapper::setRangedWorkCoordSys(int start, int count, double* data)
{
    MAZAK_CALL(m_setRangedWorkCoordSys, m_handle, start, count, data);
}

bool MazakDLLWrapper::setRangedToolOffsetComp(int start, int count, MAZ_TOFFCOMP* data)
{
    MAZAK_CALL(m_setRangedToolOffsetComp, m_handle, start, count, data);
}

bool MazakDLLWrapper::reset()
{
    MAZAK_CALL(m_reset, m_handle);
}

#undef MAZAK_CALL
