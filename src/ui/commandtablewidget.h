#ifndef COMMANDTABLEWIDGET_H
#define COMMANDTABLEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class CommandTableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommandTableWidget(QWidget *parent = nullptr);
    ~CommandTableWidget();

private slots:
    void onAddCommand();
    void onRemoveCommand();
    void onSendSelected();

private:
    void setupUi();
    void populateCommands();

    QTableWidget *m_tableWidget;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_sendButton;
};

#endif // COMMANDTABLEWIDGET_H
