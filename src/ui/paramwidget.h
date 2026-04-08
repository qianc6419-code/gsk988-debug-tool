#ifndef PARAMWIDGET_H
#define PARAMWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>

class ParamWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ParamWidget(QWidget *parent = nullptr);
signals:
    void readParam(quint16 paramNo);
    void writeParam(quint16 paramNo, const QVariant &value);
private slots:
    void onParamSelected(QTreeWidgetItem *item, int column);
    void onReadClicked();
    void onWriteClicked();
private:
    void setupUi();

    QTreeWidget *m_paramTree;
    QTableWidget *m_paramDetailTable;
    QLineEdit *m_valueEdit;
    QPushButton *m_readBtn;
    QPushButton *m_writeBtn;
};
#endif