#ifndef KNDREALTIMEWIDGET_H
#define KNDREALTIMEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>

class KndRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KndRealtimeWidget(QWidget* parent = nullptr);

    void startPolling();
    void stopPolling();
    void clearData();

private slots:
    void onPollTimeout();

private:
    void setupUI();
    void connectClientSignals();
    void startNextRequest();

    int m_pollStep = 0;
    bool m_pollBusy = false;

    QTimer* m_pollTimer;

    QLabel* m_statusLabel;
    QLabel* m_modeLabel;
    QLabel* m_programLabel;
    QLabel* m_partsLabel;

    QLabel* m_feedLabel;
    QLabel* m_feedOverrideLabel;
    QLabel* m_rapidOverrideLabel;
    QLabel* m_spindleSpeedLabel;
    QLabel* m_spindleOverrideLabel;

    QTableWidget* m_axisTable;

    QLabel* m_cycleTimeLabel;
    QLabel* m_totalTimeLabel;
    QLabel* m_axisLoadLabel;

    QPushButton* m_refreshBtn;
};

#endif // KNDREALTIMEWIDGET_H