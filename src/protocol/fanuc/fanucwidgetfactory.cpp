#include "fanucwidgetfactory.h"
#include "fanucrealtimewidget.h"
#include "fanuccommandwidget.h"
#include "fanucparsewidget.h"

QWidget* FanucWidgetFactory::createRealtimeWidget()
{
    return new FanucRealtimeWidget;
}

QWidget* FanucWidgetFactory::createCommandWidget()
{
    return new FanucCommandWidget;
}

QWidget* FanucWidgetFactory::createParseWidget()
{
    return new FanucParseWidget;
}