#include "modbuswidgetfactory.h"
#include "modbusrealtimewidget.h"
#include "modbuscommandwidget.h"
#include "modbusparsewidget.h"

QWidget* ModbusWidgetFactory::createRealtimeWidget()
{
    return new ModbusRealtimeWidget;
}

QWidget* ModbusWidgetFactory::createCommandWidget()
{
    return new ModbusCommandWidget;
}

QWidget* ModbusWidgetFactory::createParseWidget()
{
    return new ModbusParseWidget;
}
