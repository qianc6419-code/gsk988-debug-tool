#include "logwidget.h"

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , m_logTextEdit(nullptr)
    , m_saveButton(nullptr)
    , m_clearButton(nullptr)
    , m_copyButton(nullptr)
{
    setupUi();
}

LogWidget::~LogWidget()
{
}

void LogWidget::setupUi()
{
}

void LogWidget::appendLog(const QString &message)
{
    Q_UNUSED(message);
}

void LogWidget::appendLog(const QString &message, const QByteArray &data)
{
    Q_UNUSED(message);
    Q_UNUSED(data);
}

void LogWidget::clearLog()
{
}

void LogWidget::onSaveLog()
{
}

void LogWidget::onClearLog()
{
}

void LogWidget::onCopyLog()
{
}
