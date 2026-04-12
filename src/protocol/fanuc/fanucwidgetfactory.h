#ifndef FANUCWIDGETFACTORY_H
#define FANUCWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class FanucWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // FANUCWIDGETFACTORY_H