#ifndef IPROTOCOLWIDGETFACTORY_H
#define IPROTOCOLWIDGETFACTORY_H

class QWidget;

class IProtocolWidgetFactory {
public:
    virtual ~IProtocolWidgetFactory() = default;
    virtual QWidget* createRealtimeWidget() = 0;
    virtual QWidget* createCommandWidget() = 0;
    virtual QWidget* createParseWidget() = 0;
};

#endif // IPROTOCOLWIDGETFACTORY_H
