#ifndef GSK988WIDGETFACTORY_H
#define GSK988WIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class Gsk988WidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // GSK988WIDGETFACTORY_H
