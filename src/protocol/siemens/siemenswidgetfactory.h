#ifndef SIEMENSWIDGETFACTORY_H
#define SIEMENSWIDGETFACTORY_H

#include "ui/iprotocolwidgetfactory.h"

class SiemensWidgetFactory : public IProtocolWidgetFactory {
public:
    QWidget* createRealtimeWidget() override;
    QWidget* createCommandWidget() override;
    QWidget* createParseWidget() override;
};

#endif // SIEMENSWIDGETFACTORY_H
