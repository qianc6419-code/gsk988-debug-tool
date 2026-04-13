#include "protocol/knd/kndwidgetfactory.h"
#include "protocol/knd/kndrealtimewidget.h"
#include "protocol/knd/kndcommandwidget.h"
#include "protocol/knd/kndparsewidget.h"

QWidget* KndWidgetFactory::createRealtimeWidget()
{
    return new KndRealtimeWidget;
}

QWidget* KndWidgetFactory::createCommandWidget()
{
    return new KndCommandWidget;
}

QWidget* KndWidgetFactory::createParseWidget()
{
    return new KndParseWidget;
}