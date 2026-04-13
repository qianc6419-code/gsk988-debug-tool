#ifndef SIEMENSPARSEWIDGET_H
#define SIEMENSPARSEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>

class SiemensParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SiemensParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;
};

#endif // SIEMENSPARSEWIDGET_H
