#ifndef DATAPARSEWIDGET_H
#define DATAPARSEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class DataParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataParseWidget(QWidget *parent = nullptr);
private slots:
    void onParseClicked();
private:
    void setupUi();
    QString parseHexToStructure(const QString &hexInput);

    QTextEdit *m_inputEdit;
    QTextEdit *m_outputEdit;
    QPushButton *m_parseBtn;
};
#endif