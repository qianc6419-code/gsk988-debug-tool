#ifndef SIEMENSFRAMEBUILDER_H
#define SIEMENSFRAMEBUILDER_H

#include <QByteArray>
#include <QString>

class SiemensFrameBuilder
{
public:
    // ===== 握手帧 =====
    static const QByteArray HANDSHAKE_1;  // COTP Connect Request (22字节)
    static const QByteArray HANDSHAKE_2;  // S7 Communication Setup (25字节)
    static const QByteArray HANDSHAKE_3;  // S7 Setup (37字节)

    // ===== 系统信息 =====
    static const QByteArray CMD_CNC_ID;         // 0x01 CNC标识
    static const QByteArray CMD_CNC_TYPE;       // 0x02 CNC型号
    static const QByteArray CMD_VER_INFO;       // 0x03 版本信息
    static const QByteArray CMD_MANUF_DATE;     // 0x04 制造日期

    // ===== 运行状态 =====
    static const QByteArray CMD_MODE;           // 0x05 操作模式
    static const QByteArray CMD_STATUS;         // 0x06 运行状态

    // ===== 主轴 =====
    static const QByteArray CMD_SPINDLE_SET_SPEED;  // 0x07 主轴设定速度
    static const QByteArray CMD_SPINDLE_ACT_SPEED;  // 0x08 主轴实际速度
    static const QByteArray CMD_SPINDLE_RATE;       // 0x09 主轴倍率

    // ===== 进给 =====
    static const QByteArray CMD_FEED_SET_SPEED;     // 0x0A 进给设定速度
    static const QByteArray CMD_FEED_ACT_SPEED;     // 0x0B 进给实际速度
    static const QByteArray CMD_FEED_RATE;           // 0x0C 进给倍率

    // ===== 计数与时间 =====
    static const QByteArray CMD_PRODUCTS;        // 0x0D 工件数
    static const QByteArray CMD_SET_PRODUCTS;    // 0x0E 设定工件数
    static const QByteArray CMD_CYCLE_TIME;      // 0x0F 循环时间
    static const QByteArray CMD_REMAIN_TIME;     // 0x10 剩余时间

    // ===== 程序与刀具 =====
    static const QByteArray CMD_PROG_NAME;       // 0x11 程序名
    static const QByteArray CMD_PROG_CONTENT;    // 0x12 程序内容
    static const QByteArray CMD_TOOL_NUM;        // 0x13 刀具号
    static const QByteArray CMD_TOOL_D;          // 0x14 刀具半径D
    static const QByteArray CMD_TOOL_H;          // 0x15 刀具长度H
    static const QByteArray CMD_TOOL_X_LEN;      // 0x16 长度补偿X
    static const QByteArray CMD_TOOL_Z_LEN;      // 0x17 长度补偿Z
    static const QByteArray CMD_TOOL_RADIU;      // 0x18 刀具磨损半径
    static const QByteArray CMD_TOOL_EDG;        // 0x19 刀沿位置

    // ===== 坐标 =====
    static const QByteArray CMD_MACH_POS;        // 0x1A-0x1C 机械坐标
    static const QByteArray CMD_WORK_POS;        // 0x1D-0x1F 工件坐标
    static const QByteArray CMD_REMAIN_POS;      // 0x20-0x22 剩余坐标
    static const QByteArray CMD_AXIS_NAME;       // 0x23 轴名称

    // ===== 驱动诊断 =====
    static const QByteArray CMD_DRIVE_VOLTAGE;   // 0x24 母线电压
    static const QByteArray CMD_DRIVE_CURRENT;   // 0x25 驱动电流
    static const QByteArray CMD_DRIVE_LOAD1;     // 0x26 电机功率-X
    static const QByteArray CMD_DRIVE_LOAD2;     // 0x27 驱动负载2
    static const QByteArray CMD_DRIVE_LOAD3;     // 0x28 驱动负载3
    static const QByteArray CMD_DRIVE_LOAD4;     // 0x29 驱动负载4
    static const QByteArray CMD_SPINDLE_LOAD1;   // 0x2A 主轴负载1
    static const QByteArray CMD_SPINDLE_LOAD2;   // 0x2B 主轴负载2
    static const QByteArray CMD_DRIVE_TEMPER;    // 0x2C 电机温度

    // ===== 告警 =====
    static const QByteArray CMD_ALARM_NUM;       // 0x2D 告警数量
    static const QByteArray CMD_ALARM;           // 0x2E 告警信息

    // ===== 变量读写 =====
    static const QByteArray CMD_READ_R;          // 0x2F 读R变量
    static const QByteArray CMD_WRITE_R;         // 0x30 写R变量
    static const QByteArray CMD_R_DRIVER;        // 0x31 R驱动器

    // ===== 工件坐标系 =====
    static const QByteArray CMD_T_WORK_SYSTEM;  // 0x32 T工件坐标系
    static const QByteArray CMD_M_WORK_SYSTEM;  // 0x33 M工件坐标系

    // ===== PLC (独立连接) =====
    static const QByteArray PLC_HANDSHAKE_1;    // PLC 握手第1步 (22字节)
    static const QByteArray PLC_HANDSHAKE_2;    // PLC 握手第2步 (25字节)
    static const QByteArray CMD_PLC_READBYTE;   // 0x34 读DB1600寄存器

    // ===== 帧提取 =====
    // 从buffer中按TPKT头(bytes 2-3大端16位)提取完整帧
    static QByteArray extractFrame(QByteArray& buffer);

    // ===== 参数化命令构建 =====
    // 位置命令: 修改base帧偏移26为axisFlag
    static QByteArray buildPositionCmd(const QByteArray& base, quint8 axisFlag);
    // 告警命令: 修改偏移26为alarmIndex
    static QByteArray buildAlarmCmd(int alarmIndex);
    // 读R变量: 修改偏移25-26为变量地址
    static QByteArray buildReadRCmd(int rAddr);
    // 写R变量: 修改地址和double值
    static QByteArray buildWriteRCmd(int rAddr, double value);
    // R驱动器: 修改偏移22(轴), 24(地址), 26(索引)
    static QByteArray buildRDriverCmd(quint8 axisFlag, quint8 addrFlag, quint8 indexFlag);
    // PLC 读 DB1600：修改最后字节为偏移量 (byteOffset * 8)
    static QByteArray buildPLCReadCmd(int byteOffset);

    // ===== 响应解析 =====
    // String: offset 25, UTF-8, 去\0
    static QString parseString(const QByteArray& frame);
    // Float: offset 25, 4字节小端(reverse后转float)
    static float parseFloat(const QByteArray& frame);
    // Double: frame[3]==33时offset 25取8字节, 否则offset 0
    static double parseDouble(const QByteArray& frame);
    // Int32: offset 25, 4字节
    static qint32 parseInt32(const QByteArray& frame);
    // Int16: offset 25, 2字节
    static qint16 parseInt16(const QByteArray& frame);
    // Mode: offset 24/25/31 → JOG/MDA/AUTO/REFPOINT/OTHER
    static QString parseMode(const QByteArray& frame);
    // Status: offset 24/25/31 → RESET/STOP/START/OTHER
    static QString parseStatus(const QByteArray& frame);

    // ===== 数据类型编码 =====
    // double转8字节小端
    static QByteArray encodeDouble(double value);
    // float转4字节(先memcpy再reverse用于S7)
    static QByteArray encodeFloat(float value);
};

#endif // SIEMENSFRAMEBUILDER_H
