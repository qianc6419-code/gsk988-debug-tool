#ifndef MODBUSCOMMANDWIDGET_H
#define MODBUSCOMMANDWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVector>
#include "protocol/iprotocol.h"
#include "modbusprotocol.h"

class ModbusCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusCommandWidget(QWidget* parent = nullptr);

    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void populateTable();
    void applyFilter(const QString& text);
    void updateParamInputs();

    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QTableWidget* m_table;
    QTextEdit* m_resultDisplay;

    // Parameter inputs (right panel)
    QSpinBox* m_unitIdSpin;
    QWidget* m_paramPanel;
    QLineEdit* m_addrEdit;
    QLineEdit* m_qtyEdit;
    QLineEdit* m_writeAddrEdit;
    QLineEdit* m_writeQtyEdit;
    QLineEdit* m_valueEdit;
    QComboBox* m_dataTypeCombo;
    QComboBox* m_byteOrderCombo;

    QVector<ModbusCommandDef> m_commands;
};

#endif // MODBUSCOMMANDWIDGET_H
