#ifndef FANUCCOMMANDWIDGET_H
#define FANUCCOMMANDWIDGET_H

#include <QWidget>
#include <QVector>
#include "fanucprotocol.h"

class QTableWidget;
class QTextEdit;
class QLineEdit;
class QComboBox;

class FanucCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FanucCommandWidget(QWidget* parent = nullptr);
    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void populateTable();
    void applyFilter(const QString& text);
    void updateParamVisibility(quint8 cmdCode);

    QTableWidget* m_table;
    QTextEdit* m_resultDisplay;
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QVector<FanucCommandDef> m_commands;

    // Parameter inputs
    QLineEdit* m_addrEdit;       // for macro addr, param no, PMC addr
    QLineEdit* m_valueEdit;      // for write values
    QComboBox* m_addrTypeCombo;  // for PMC addr type (G/F/Y/X/...)
    QComboBox* m_dataTypeCombo;  // for PMC data type (byte/word/long)
};

#endif // FANUCCOMMANDWIDGET_H
