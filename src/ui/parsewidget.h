#ifndef PARSEWIDGET_H
#define PARSEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class Gsk988Protocol;

class ParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;

    Gsk988Protocol* m_protocol;
};

#endif // PARSEWIDGET_H
