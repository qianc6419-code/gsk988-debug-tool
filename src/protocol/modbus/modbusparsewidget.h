#ifndef MODBUSPARSEWIDGET_H
#define MODBUSPARSEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class ModbusParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;
};

#endif // MODBUSPARSEWIDGET_H
