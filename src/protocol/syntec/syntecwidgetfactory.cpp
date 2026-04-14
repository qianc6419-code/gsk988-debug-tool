#include "syntecwidgetfactory.h"
#include "syntecrealtimewidget.h"
#include "synteccommandwidget.h"
#include "syntecparsewidget.h"

QWidget* SyntecWidgetFactory::createRealtimeWidget()
{
    return new SyntecRealtimeWidget;
}

QWidget* SyntecWidgetFactory::createCommandWidget()
{
    return new SyntecCommandWidget;
}

QWidget* SyntecWidgetFactory::createParseWidget()
{
    return new SyntecParseWidget;
}
