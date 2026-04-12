#ifndef FANUCPARSEWIDGET_H
#define FANUCPARSEWIDGET_H

#include <QWidget>

class QTextEdit;
class QPushButton;
class FanucParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FanucParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;
};

#endif // FANUCPARSEWIDGET_H