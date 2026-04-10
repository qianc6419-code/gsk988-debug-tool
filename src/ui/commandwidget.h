#ifndef COMMANDWIDGET_H
#define COMMANDWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVector>
#include "protocol/gsk988protocol.h"

class CommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CommandWidget(QWidget* parent = nullptr);

    void showResponse(const struct ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void populateTable();
    void applyFilter(const QString& text);

    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QTableWidget* m_table;
    QTextEdit* m_resultDisplay;

    QVector<CommandDef> m_commands;
};

#endif // COMMANDWIDGET_H
