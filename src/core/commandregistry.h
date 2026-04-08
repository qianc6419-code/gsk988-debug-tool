#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include <QObject>
#include <QMap>
#include <QString>

struct CommandInfo
{
    quint8 commandCode;
    QString name;
    QString description;
    int paramCount;
};

class CommandRegistry : public QObject
{
    Q_OBJECT

public:
    static CommandRegistry& instance();

    void registerCommand(quint8 code, const QString &name, const QString &description, int paramCount);
    CommandInfo getCommand(quint8 code) const;
    QList<CommandInfo> getAllCommands() const;
    QString getCommandName(quint8 code) const;

signals:
    void commandRegistered(quint8 code, const QString &name);

private:
    explicit CommandRegistry(QObject *parent = nullptr);
    ~CommandRegistry();
    CommandRegistry(const CommandRegistry&) = delete;
    CommandRegistry& operator=(const CommandRegistry&) = delete;

    QMap<quint8, CommandInfo> m_commands;
};

#endif // COMMANDREGISTRY_H
