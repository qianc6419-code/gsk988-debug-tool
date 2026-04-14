#ifndef SYNTECREALTIMEWIDGET_H
#define SYNTECREALTIMEWIDGET_H

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

class SyntecRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyntecRealtimeWidget(QWidget* parent = nullptr);

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

    QCheckBox* m_autoRefreshCheck;
    QComboBox* m_intervalCombo;
    QPushButton* m_manualRefreshBtn;
    QTextEdit* m_hexDisplay;

    QGroupBox* m_statusGroup;
    QLabel* m_statusLabel;
    QLabel* m_modeLabel;
    QLabel* m_progNameLabel;
    QLabel* m_emgLabel;
    QLabel* m_alarmLabel;
    QLabel* m_productsLabel;

    QGroupBox* m_feedGroup;
    QLabel* m_feedRateLabel;
    QLabel* m_feedOverrideLabel;
    QLabel* m_spindleSpeedLabel;
    QLabel* m_spindleOverrideLabel;

    QGroupBox* m_coordGroup;
    QLabel* m_machCoord[3];
    QLabel* m_relCoord[3];

    QGroupBox* m_timeGroup;
    QLabel* m_runTimeLabel;
    QLabel* m_cutTimeLabel;
    QLabel* m_cycleTimeLabel;

    QTimer* m_cycleDelayTimer;
    QTimer* m_interPollTimer;
    int m_pollIndex;
    bool m_cycleActive;
    int m_decPoint;

    enum PollIndex {
        PI_STATUS = 0,
        PI_MODE,
        PI_PROGNAME,
        PI_ALARM,
        PI_EMG,
        PI_DECPOINT,
        PI_REL_COORD,
        PI_MACH_COORD,
        PI_FEED_OVERRIDE,
        PI_SPINDLE_OVERRIDE,
        PI_SPINDLE_SPEED,
        PI_FEED_RATE_ORI,
        PI_RUN_TIME,
        PI_CUT_TIME,
        PI_CYCLE_TIME,
        PI_PRODUCTS,
        PI_COUNT
    };

    struct PollItem {
        quint8 cmdCode;
        QByteArray params;
    };
    static const PollItem pollItems[];
};

#endif // SYNTECREALTIMEWIDGET_H
