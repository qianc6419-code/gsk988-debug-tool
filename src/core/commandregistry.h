#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>

struct CommandInfo
{
    quint8 cmd;           // 命令码
    quint8 sub;           // 子码
    quint8 type;          // 类型
    QString name;         // 命令名称
    QString description;  // 说明
    QString category;     // 类别: 读/写/控制
    QString function;     // 功能分组
    bool hasParams;       // 是否有参数
    QString paramHint;    // 参数提示
    QVariant paramValue;
};

class CommandRegistry : public QObject
{
    Q_OBJECT
public:
    static CommandRegistry* instance();
    const QVector<CommandInfo>& allCommands() const { return m_commands; }
    CommandInfo getCommand(quint8 cmd, quint8 sub, quint8 type) const;
    QVector<CommandInfo> filterByCategory(const QString &category) const;
    QVector<CommandInfo> filterByFunction(const QString &function) const;
    QVector<CommandInfo> search(const QString &keyword) const;
signals:
    void commandChanged();
private:
    explicit CommandRegistry(QObject *parent = nullptr);
    void registerAllCommands();
    QVector<CommandInfo> m_commands;
};
#endif