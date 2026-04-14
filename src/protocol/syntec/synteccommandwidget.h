#ifndef SYNTECCOMMANDWIDGET_H
#define SYNTECCOMMANDWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include "protocol/iprotocol.h"

class SyntecCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyntecCommandWidget(QWidget* parent = nullptr);

    void showResponse(const ParsedResponse& resp, const QString& interpretation);

signals:
    void sendCommand(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();

    QComboBox* m_cmdCombo;
    QPushButton* m_sendBtn;
    QTextEdit* m_resultDisplay;
};

#endif // SYNTECCOMMANDWIDGET_H
