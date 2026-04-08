#include "realtimewidget.h"

RealtimeWidget::RealtimeWidget(QWidget *parent)
    : QWidget(parent)
    , m_tableWidget(nullptr)
    , m_refreshTimer(nullptr)
{
    setupUi();
}

RealtimeWidget::~RealtimeWidget()
{
}

void RealtimeWidget::setupUi()
{
}

void RealtimeWidget::onRefreshTimer()
{
}

void RealtimeWidget::onClearData()
{
}

void RealtimeWidget::updateDisplay()
{
}
