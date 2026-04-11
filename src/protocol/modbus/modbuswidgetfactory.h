#ifndef MODBUSWIDGETFACTORY_H
#define MODBUSWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class ModbusWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // MODBUSWIDGETFACTORY_H
