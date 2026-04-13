#include "siemenswidgetfactory.h"
#include "siemensrealtimewidget.h"
#include "siemenscommandwidget.h"
#include "siemensparsewidget.h"

QWidget* SiemensWidgetFactory::createRealtimeWidget()
{
    return new SiemensRealtimeWidget;
}

QWidget* SiemensWidgetFactory::createCommandWidget()
{
    return new SiemensCommandWidget;
}

QWidget* SiemensWidgetFactory::createParseWidget()
{
    return new SiemensParseWidget;
}
