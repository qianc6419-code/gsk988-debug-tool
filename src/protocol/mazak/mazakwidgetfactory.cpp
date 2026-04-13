#include "protocol/mazak/mazakwidgetfactory.h"
#include "protocol/mazak/mazakrealtimewidget.h"
#include "protocol/mazak/mazakcommandwidget.h"
#include "protocol/mazak/mazakparsewidget.h"

QWidget* MazakWidgetFactory::createRealtimeWidget()
{
    return new MazakRealtimeWidget;
}

QWidget* MazakWidgetFactory::createCommandWidget()
{
    return new MazakCommandWidget;
}

QWidget* MazakWidgetFactory::createParseWidget()
{
    return new MazakParseWidget;
}
