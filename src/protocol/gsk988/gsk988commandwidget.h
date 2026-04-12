#ifndef GSK988COMMANDWIDGET_H
#define GSK988COMMANDWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVector>
#include "protocol/iprotocol.h"
#include "gsk988protocol.h"

class Gsk988CommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit Gsk988CommandWidget(QWidget* parent = nullptr);

    void showResponse(const ParsedResponse& resp, const QString& interpretation);

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

    QVector<Gsk988CommandDef> m_commands;
};

#endif // GSK988COMMANDWIDGET_H
