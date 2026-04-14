#ifndef SYNTECWIDGETFACTORY_H
#define SYNTECWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class SyntecWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // SYNTECWIDGETFACTORY_H
