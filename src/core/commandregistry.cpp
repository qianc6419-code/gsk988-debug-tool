#include "commandregistry.h"

CommandRegistry::CommandRegistry(QObject *parent)
    : QObject(parent)
{
}

CommandRegistry::~CommandRegistry()
{
}

CommandRegistry& CommandRegistry::instance()
{
    static CommandRegistry instance;
    return instance;
}

void CommandRegistry::registerCommand(quint8 code, const QString &name, const QString &description, int paramCount)
{
    Q_UNUSED(code);
    Q_UNUSED(name);
    Q_UNUSED(description);
    Q_UNUSED(paramCount);
}

CommandInfo CommandRegistry::getCommand(quint8 code) const
{
    Q_UNUSED(code);
    return CommandInfo();
}

QList<CommandInfo> CommandRegistry::getAllCommands() const
{
    return QList<CommandInfo>();
}

QString CommandRegistry::getCommandName(quint8 code) const
{
    Q_UNUSED(code);
    return QString();
}
