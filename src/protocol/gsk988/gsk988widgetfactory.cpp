#include "gsk988widgetfactory.h"
#include "gsk988realtimewidget.h"
#include "gsk988commandwidget.h"
#include "gsk988parsewidget.h"

QWidget* Gsk988WidgetFactory::createRealtimeWidget()
{
    return new Gsk988RealtimeWidget;
}

QWidget* Gsk988WidgetFactory::createCommandWidget()
{
    return new Gsk988CommandWidget;
}

QWidget* Gsk988WidgetFactory::createParseWidget()
{
    return new Gsk988ParseWidget;
}
