#ifndef SIEMENSREALTIMEWIDGET_H
#define SIEMENSREALTIMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include "protocol/iprotocol.h"

class SiemensRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SiemensRealtimeWidget(QWidget* parent = nullptr);

    void updateData(const ParsedResponse& resp);
    void startPolling();
    void stopPolling();
    void clearData();

    enum PollIndex {
        // Row 0 — 运行状态 + 主轴 + 进给
        PI_MODE = 0,            // 0x05
        PI_STATUS,              // 0x06
        PI_SPINDLE_SET_SPEED,   // 0x07
        PI_SPINDLE_ACT_SPEED,   // 0x08
        PI_SPINDLE_RATE,        // 0x09
        PI_FEED_SET_SPEED,      // 0x0A
        PI_FEED_ACT_SPEED,      // 0x0B
        PI_FEED_RATE,           // 0x0C
        PI_ALARM_NUM,           // 0x2D
        // Row 1 — 系统信息 + 计数时间 + 程序与刀具
        PI_CNC_ID,              // 0x01
        PI_CNC_TYPE,            // 0x02
        PI_VER_INFO,            // 0x03
        PI_PRODUCTS,            // 0x0D
        PI_SET_PRODUCTS,        // 0x0E
        PI_CYCLE_TIME,          // 0x0F
        PI_REMAIN_TIME,         // 0x10
        PI_PROG_NAME,           // 0x11
        PI_TOOL_NUM,            // 0x13
        PI_TOOL_D,              // 0x14
        PI_TOOL_H,              // 0x15
        PI_TOOL_X_LEN,          // 0x16
        PI_TOOL_Z_LEN,          // 0x17
        PI_TOOL_RADIU,          // 0x18
        PI_TOOL_EDG,            // 0x19
        // Row 2 — 坐标
        PI_AXIS_NAME,           // 0x23
        PI_MACH_X,              // 0x1A
        PI_MACH_Y,              // 0x1B
        PI_MACH_Z,              // 0x1C
        PI_WORK_X,              // 0x1D
        PI_WORK_Y,              // 0x1E
        PI_WORK_Z,              // 0x1F
        PI_REMAIN_X,            // 0x20
        PI_REMAIN_Y,            // 0x21
        PI_REMAIN_Z,            // 0x22
        // Row 2 — 驱动诊断
        PI_DRIVE_VOLTAGE,       // 0x24
        PI_DRIVE_CURRENT,       // 0x25
        PI_DRIVE_LOAD1,         // 0x26
        PI_SPINDLE_LOAD1,       // 0x27
        PI_SPINDLE_LOAD2,       // 0x28
        PI_DRIVE_TEMPER,        // 0x2C
        PI_COUNT
    };

    struct PollItem {
        quint8 cmdCode;
        QByteArray params;
    };
    static const PollItem pollItems[];

public slots:
    void appendHexDisplay(const QByteArray& data, bool isSend);

signals:
    void pollRequest(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void startCycle();

    void setLabelOK(QLabel* label, const QString& text);
    void setLabelError(QLabel* label, const QString& err);
    void resetLabel(QLabel* label);

    // Controls
    QCheckBox* m_autoRefreshCheck;
    QComboBox* m_intervalCombo;
    QPushButton* m_manualRefreshBtn;
    QTextEdit* m_hexDisplay;

    // Row 0 — 运行状态
    QGroupBox* m_runStatusGroup;
    QLabel* m_modeLabel;
    QLabel* m_runStatusLabel;
    QLabel* m_alarmIndicator;

    // Row 0 — 主轴
    QGroupBox* m_spindleGroup;
    QLabel* m_spindleActSpeedLabel;
    QLabel* m_spindleRateLabel;
    QLabel* m_spindleSetSpeedLabel;

    // Row 0 — 进给
    QGroupBox* m_feedGroup;
    QLabel* m_feedActSpeedLabel;
    QLabel* m_feedRateLabel;
    QLabel* m_feedSetSpeedLabel;

    // Row 1 — 系统信息
    QGroupBox* m_sysInfoGroup;
    QLabel* m_cncTypeLabel;
    QLabel* m_verInfoLabel;
    QLabel* m_cncIdLabel;

    // Row 1 — 计数时间
    QGroupBox* m_counterTimeGroup;
    QLabel* m_productsLabel;
    QLabel* m_setProductsLabel;
    QLabel* m_cycleTimeLabel;
    QLabel* m_remainTimeLabel;

    // Row 1 — 程序与刀具
    QGroupBox* m_progToolGroup;
    QLabel* m_progNameLabel;
    QLabel* m_toolNumLabel;
    QLabel* m_toolDLabel;
    QLabel* m_toolHLabel;
    QLabel* m_toolRadiuLabel;
    QLabel* m_toolXLenLabel;
    QLabel* m_toolZLenLabel;
    QLabel* m_toolEdgLabel;

    // Row 2 — 告警
    QGroupBox* m_alarmGroup;
    QLabel* m_alarmTextLabel;

    // Row 2 — 坐标
    QGroupBox* m_coordGroup;
    QLabel* m_axisNameLabels[3];
    QLabel* m_posMach[3];
    QLabel* m_posWork[3];
    QLabel* m_posRemain[3];

    // Row 2 — 驱动诊断
    QGroupBox* m_driveGroup;
    QLabel* m_driveVoltageLabel;
    QLabel* m_driveCurrentLabel;
    QLabel* m_driveLoad1Label;
    QLabel* m_spindleLoad1Label;
    QLabel* m_spindleLoad2Label;
    QLabel* m_driveTemperLabel;

    // Row 3 — PLC 告警
    QGroupBox* m_plcAlarmGroup;
    QLabel* m_plcAlarmLabel;

    // 告警状态缓存
    int m_alarmCount;
    QStringList m_alarmMessages;

    // Polling state
    QTimer* m_cycleDelayTimer;
    int m_pollIndex;
    bool m_cycleActive;
};

#endif // SIEMENSREALTIMEWIDGET_H
