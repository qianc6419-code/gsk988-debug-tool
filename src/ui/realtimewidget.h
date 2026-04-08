#ifndef REALTIMEWIDGET_H
#define REALTIMEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QTimer>

class RealtimeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RealtimeWidget(QWidget *parent = nullptr);
    ~RealtimeWidget();

private slots:
    void onRefreshTimer();
    void onClearData();

private:
    void setupUi();
    void updateDisplay();

    QTableWidget *m_tableWidget;
    QTimer *m_refreshTimer;
};

#endif // REALTIMEWIDGET_H
