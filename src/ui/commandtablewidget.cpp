#include "commandtablewidget.h"

CommandTableWidget::CommandTableWidget(QWidget *parent)
    : QWidget(parent)
    , m_tableWidget(nullptr)
    , m_addButton(nullptr)
    , m_removeButton(nullptr)
    , m_sendButton(nullptr)
{
    setupUi();
}

CommandTableWidget::~CommandTableWidget()
{
}

void CommandTableWidget::setupUi()
{
}

void CommandTableWidget::populateCommands()
{
}

void CommandTableWidget::onAddCommand()
{
}

void CommandTableWidget::onRemoveCommand()
{
}

void CommandTableWidget::onSendSelected()
{
}
