#include "protocol/mazak/mazakframebuilder.h"

namespace MazakFrameBuilder {

QString errorToString(int code)
{
    switch (code) {
    case MazakError::OK:        return QString("成功");
    case MazakError::SOCK:      return QString("套接字错误");
    case MazakError::HNDL:      return QString("句柄无效");
    case MazakError::CLIMAX:    return QString("超过最大客户端数");
    case MazakError::SERVERMAX: return QString("超过服务器最大连接数");
    case MazakError::VER:       return QString("版本不匹配");
    case MazakError::BUSY:      return QString("资源忙");
    case MazakError::RUNNING:   return QString("NC 正在运行");
    case MazakError::OVER:      return QString("数据溢出");
    case MazakError::NONE:      return QString("数据不存在");
    case MazakError::TYPE:      return QString("数据类型错误");
    case MazakError::EDIT:      return QString("编辑中");
    case MazakError::PROSIZE:   return QString("程序大小超限");
    case MazakError::PRONUM:    return QString("程序号错误");
    case MazakError::ARG:       return QString("参数错误");
    case MazakError::SYS:       return QString("系统错误");
    case MazakError::FUNC:      return QString("功能不可用");
    case MazakError::TIMEOUT:   return QString("超时");
    default:                    return QString("未知错误(%1)").arg(code);
    }
}

QString fixedString(const char* data, int maxSize)
{
    return QString::fromLocal8Bit(data, static_cast<int>(strnlen(data, maxSize)));
}

} // namespace MazakFrameBuilder
