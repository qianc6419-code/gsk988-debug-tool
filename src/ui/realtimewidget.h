#ifndef REALTIMEWIDGET_H
#define REALTIMEWIDGET_H

#include <QWidget>
#include <QMap>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>

class RealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RealtimeWidget(QWidget *parent = nullptr);
    void updateDeviceInfo(const QMap<QString, QVariant> &data);
    void updateMachineStatus(const QMap<QString, QVariant> &data);
    void updateCoordinates(const QMap<QString, QVariant> &data);
    void updateAlarms(const QMap<QString, QVariant> &data);
    void appendRawData(const QByteArray &hex);
public slots:
    void clear();
private:
    void setupUi();
    QString hexToDisplay(const QByteArray &data);

    QGroupBox *m_statusCard;
    QGroupBox *m_machineCoordCard;
    QGroupBox *m_workCoordCard;
    QGroupBox *m_alarmCard;
    QLabel *m_runStateLabel;
    QLabel *m_modeLabel;
    QLabel *m_feedLabel;
    QLabel *m_spindleLabel;
    QLabel *m_toolLabel;
    QLabel *m_mxLabel, *m_myLabel, *m_mzLabel;
    QLabel *m_wxLabel, *m_wyLabel, *m_wzLabel;
    QLabel *m_alarmCountLabel;
    QLabel *m_alarmDetailLabel;
    QTextEdit *m_rawDataEdit;
};
#endif
