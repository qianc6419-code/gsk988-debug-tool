#ifndef FANUCREALTIMEWIDGET_H
#define FANUCREALTIMEWIDGET_H

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

class FanucRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FanucRealtimeWidget(QWidget* parent = nullptr);

    void updateData(const ParsedResponse& resp);
    void startPolling();
    void stopPolling();
    void clearData();

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

    // System info group
    QGroupBox* m_sysInfoGroup;
    QLabel* m_ncTypeLabel;
    QLabel* m_deviceTypeLabel;

    // Run status group
    QGroupBox* m_runStatusGroup;
    QLabel* m_modeLabel;
    QLabel* m_runStatusLabel;
    QLabel* m_emgLabel;
    QLabel* m_almLabel;

    // Spindle group
    QGroupBox* m_spindleGroup;
    QLabel* m_spindleSpeedLabel;
    QLabel* m_spindleLoadLabel;
    QLabel* m_spindleOverrideLabel;
    QLabel* m_spindleSpeedSetLabel;

    // Feed group
    QGroupBox* m_feedGroup;
    QLabel* m_feedRateLabel;
    QLabel* m_feedOverrideLabel;
    QLabel* m_feedRateSetLabel;

    // Counters/Time group
    QGroupBox* m_counterTimeGroup;
    QLabel* m_productsLabel;
    QLabel* m_runTimeLabel;
    QLabel* m_cutTimeLabel;
    QLabel* m_cycleTimeLabel;
    QLabel* m_powerOnTimeLabel;

    // Program/Tool group
    QGroupBox* m_progToolGroup;
    QLabel* m_progNumLabel;
    QLabel* m_toolNumLabel;

    // Alarm group
    QGroupBox* m_alarmGroup;
    QTextEdit* m_alarmDisplay;

    // Coordinates group (4 rows x 3 columns)
    QGroupBox* m_coordGroup;
    QLabel* m_posAbs[3];
    QLabel* m_posMach[3];
    QLabel* m_posRel[3];
    QLabel* m_posRes[3];

    // Polling state
    QTimer* m_cycleDelayTimer;
    int m_pollIndex;
    bool m_cycleActive;

    // Poll item definitions
    enum PollIndex {
        PI_SYSINFO = 0,        // 0x01
        PI_RUNINFO,            // 0x02
        PI_SPINDLE_SPEED,      // 0x03
        PI_SPINDLE_LOAD,       // 0x04
        PI_SPINDLE_OVERRIDE,   // 0x05
        PI_SPINDLE_SPEED_SET,  // 0x06
        PI_FEED_RATE,          // 0x07
        PI_FEED_OVERRIDE,      // 0x08
        PI_FEED_RATE_SET,      // 0x09
        PI_PRODUCTS,           // 0x0A
        PI_RUN_TIME,           // 0x0B
        PI_CUT_TIME,           // 0x0C
        PI_CYCLE_TIME,         // 0x0D
        PI_POWERON_TIME,       // 0x0E
        PI_PROG_NUM,           // 0x0F
        PI_TOOL_NUM,           // 0x10
        PI_ALARM,              // 0x11
        PI_POS_ABS,            // 0x12
        PI_POS_MACH,           // 0x13
        PI_POS_REL,            // 0x14
        PI_POS_RES,            // 0x15
        PI_COUNT
    };

    struct PollItem {
        quint8 cmdCode;
        QByteArray params;
    };
    static const PollItem pollItems[];
};

#endif // FANUCREALTIMEWIDGET_H
