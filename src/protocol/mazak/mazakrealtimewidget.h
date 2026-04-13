#ifndef MAZAKREALTIMEWIDGET_H
#define MAZAKREALTIMEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>

class MazakRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MazakRealtimeWidget(QWidget* parent = nullptr);

    void startPolling();
    void stopPolling();
    void clearData();

private slots:
    void onPollTimeout();

private:
    void setupUI();
    void updateStatusLabel(const struct MTC_ONE_RUNNING_INFO& ri);
    void updateModeLabel(const struct MTC_ONE_RUNNING_INFO& ri);
    void updateAxisTable(const struct MTC_AXIS_INFO& ai);
    void updateSpindleLabels(const struct MTC_SPINDLE_INFO& si);
    void updateAlarmLabel(const struct MAZ_ALARMALL& al);
    void updateTimeLabels(const struct MAZ_ACCUM_TIME& at);

    QTimer* m_pollTimer;

    // 第1行: 系统状态
    QLabel* m_statusLabel;
    QLabel* m_modeLabel;
    QLabel* m_programLabel;
    QLabel* m_partsLabel;

    // 第2行: 主轴 + 进给
    QLabel* m_setSpeedLabel;
    QLabel* m_actSpeedLabel;
    QLabel* m_spLoadLabel;
    QLabel* m_spOverrideLabel;
    QLabel* m_feedLabel;
    QLabel* m_feedOverrideLabel;
    QLabel* m_rapidOverrideLabel;

    // 第3行: 坐标
    QTableWidget* m_axisTable;

    // 底部信息栏
    QLabel* m_alarmLabel;
    QLabel* m_toolLabel;
    QLabel* m_powerOnLabel;
    QLabel* m_runTimeLabel;

    QPushButton* m_refreshBtn;
};

#endif // MAZAKREALTIMEWIDGET_H
