#ifndef SYNTECPARSEWIDGET_H
#define SYNTECPARSEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>

class SyntecParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyntecParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QComboBox* m_typeCombo;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;
};

#endif // SYNTECPARSEWIDGET_H
