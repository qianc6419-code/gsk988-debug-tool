#ifndef MAZAKWIDGETFACTORY_H
#define MAZAKWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class MazakWidgetFactory : public IProtocolWidgetFactory
{
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // MAZAKWIDGETFACTORY_H
