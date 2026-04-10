#ifndef REALTIMEWIDGET_H
#define REALTIMEWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QScrollArea>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include "protocol/gsk988protocol.h"

class RealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RealtimeWidget(QWidget* parent = nullptr);

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
    void updateDeviceCard(const ParsedResponse& resp);
    void updateSpeedCard(const ParsedResponse& resp);
    void updateCoordCard(const ParsedResponse& resp);
    void updateMachiningCard(const ParsedResponse& resp);
    void updateAlarmCard(const ParsedResponse& resp);
    void updateProgramCard(const ParsedResponse& resp);

    void setLabelOK(QLabel* label, const QString& text);
    void setLabelError(QLabel* label, const QString& err);
    void resetLabel(QLabel* label);

    // Controls
    QCheckBox* m_autoRefreshCheck;
    QComboBox* m_intervalCombo;
    QPushButton* m_manualRefreshBtn;

    // Cards
    QGroupBox* m_deviceCard;
    QGroupBox* m_feedCard;
    QGroupBox* m_spindleCard;
    QGroupBox* m_overrideCard;
    QGroupBox* m_machineCoordCard;
    QGroupBox* m_absCoordCard;
    QGroupBox* m_relCoordCard;
    QGroupBox* m_remainCoordCard;
    QGroupBox* m_machiningCard;
    QGroupBox* m_programCard;
    QGroupBox* m_modalCard;
    QGroupBox* m_alarmCard;

    // Device status
    QLabel* m_runModeLabel;
    QLabel* m_runStatusLabel;
    QLabel* m_pauseLabel;

    // Feed speed
    QLabel* m_feedProgLabel;
    QLabel* m_feedActualLabel;
    QLabel* m_feedOverrideLabel;

    // Spindle
    QLabel* m_spindleProgLabel;
    QLabel* m_spindleActualLabel;
    QLabel* m_spindleOverrideLabel;

    // Other overrides
    QLabel* m_rapidOverrideLabel;
    QLabel* m_manualOverrideLabel;
    QLabel* m_handwheelOverrideLabel;

    // Coordinates (4 groups x 3 axes)
    QLabel* m_machineCoord[3];
    QLabel* m_absCoord[3];
    QLabel* m_relCoord[3];
    QLabel* m_remainCoord[3];

    // Machining
    QLabel* m_pieceCountLabel;
    QLabel* m_runTimeLabel;
    QLabel* m_cutTimeLabel;
    QLabel* m_toolNoLabel;

    // Program
    QLabel* m_programNameLabel;
    QLabel* m_segmentLabel;
    QLabel* m_execLineLabel;

    // Modal
    QLabel* m_gModalLabel;
    QLabel* m_mModalLabel;

    // Alarm
    QLabel* m_errorCountLabel;
    QLabel* m_warnCountLabel;

    QTextEdit* m_hexDisplay;

    // Polling
    QTimer* m_cycleDelayTimer; // waits between auto-refresh cycles
    int m_pollIndex;
    int m_pollCount;
    bool m_cycleActive;  // true while cycling through all items

    void startCycle();

    // Poll item indices
    enum {
        PI_RUN_MODE = 0,
        PI_RUN_STATUS,
        PI_PAUSE,
        PI_FEED_PROG,
        PI_FEED_ACTUAL,
        PI_FEED_OVERRIDE,
        PI_SPINDLE_PROG,
        PI_SPINDLE_ACTUAL,
        PI_SPINDLE_OVERRIDE,
        PI_RAPID_OVERRIDE,
        PI_MANUAL_OVERRIDE,
        PI_HANDWHEEL_OVERRIDE,
        PI_COORD_MACHINE,
        PI_COORD_ABS,
        PI_COORD_REL,
        PI_COORD_REMAIN,
        PI_PIECE_COUNT,
        PI_TOOL_NO,
        PI_RUN_TIME,
        PI_CUT_TIME,
        PI_PROGRAM_NAME,
        PI_SEGMENT,
        PI_EXEC_LINE,
        PI_G_MODAL,
        PI_M_MODAL,
        PI_ALARM,
        PI_COUNT
    };

    struct PollItem {
        quint8 cmdCode;
        QByteArray params;
    };
    static const PollItem pollItems[];
};

#endif // REALTIMEWIDGET_H
