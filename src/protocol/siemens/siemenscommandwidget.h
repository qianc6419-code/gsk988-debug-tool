#ifndef SIEMENSCOMMANDWIDGET_H
#define SIEMENSCOMMANDWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QStackedWidget>
#include "protocol/iprotocol.h"

class SiemensCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SiemensCommandWidget(QWidget* parent = nullptr);

    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void populateCommandTable();
    void onCurrentCellChanged(int row, int col, int prevRow, int prevCol);
    void onSendClicked();

    QTableWidget* m_cmdTable;
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;

    // Parameter panel
    QStackedWidget* m_paramStack;
    QWidget* m_noParamPage;
    QWidget* m_alarmParamPage;
    QWidget* m_readRParamPage;
    QWidget* m_writeRParamPage;
    QWidget* m_rDriverParamPage;

    // Alarm params
    QSpinBox* m_alarmIndexSpin;

    // Read R params
    QSpinBox* m_readRAddrSpin;

    // Write R params
    QSpinBox* m_writeRAddrSpin;
    QDoubleSpinBox* m_writeRValueSpin;

    // R Driver params
    QSpinBox* m_rDriverAxisSpin;
    QSpinBox* m_rDriverAddrSpin;
    QSpinBox* m_rDriverIndexSpin;

    // PLC params
    QWidget* m_plcParamPage;
    QSpinBox* m_plcOffsetSpin;

    QPushButton* m_sendBtn;
    QTextEdit* m_resultDisplay;
};

#endif // SIEMENSCOMMANDWIDGET_H
