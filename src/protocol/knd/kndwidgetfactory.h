#ifndef KNDWIDGETFACTORY_H
#define KNDWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class KndWidgetFactory : public IProtocolWidgetFactory
{
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // KNDWIDGETFACTORY_H