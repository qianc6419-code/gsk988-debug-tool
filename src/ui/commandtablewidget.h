#ifndef COMMANDTABLEWIDGET_H
#define COMMANDTABLEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include "core/commandregistry.h"

class CommandTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CommandTableWidget(QWidget *parent = nullptr);
    void populateCommands();
signals:
    void commandSendRequested(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data);
private slots:
    void onSearchChanged(const QString &text);
    void onCategoryChanged(int index);
    void onFunctionChanged(int index);
    void onSendClicked(int row);
private:
    void filterCommands();
    QString getParamFromRow(int row, const CommandInfo &cmd);

    QLineEdit *m_searchEdit;
    QComboBox *m_categoryCombo;
    QComboBox *m_functionCombo;
    QTableWidget *m_table;
    QVector<CommandInfo> m_allCommands;
    QVector<CommandInfo> m_filteredCommands;
};
#endif
