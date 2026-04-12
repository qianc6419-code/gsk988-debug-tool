#ifndef GSK988PARSEWIDGET_H
#define GSK988PARSEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include "gsk988protocol.h"
#include "gsk988framebuilder.h"

class Gsk988ParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit Gsk988ParseWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    void doParse();

    QTextEdit* m_hexInput;
    QPushButton* m_parseBtn;
    QTextEdit* m_resultDisplay;

    Gsk988Protocol* m_protocol;
};

#endif // GSK988PARSEWIDGET_H
