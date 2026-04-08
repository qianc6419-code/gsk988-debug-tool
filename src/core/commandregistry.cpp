#include "commandregistry.h"
#include <QVector>
#include <QString>

CommandRegistry::CommandRegistry(QObject *parent)
    : QObject(parent)
{
    registerAllCommands();
}

CommandRegistry* CommandRegistry::instance()
{
    static CommandRegistry instance;
    return &instance;
}

void CommandRegistry::registerAllCommands()
{
    m_commands.clear();

    // Read commands
    m_commands.append({0x01, 0x00, 0x01, "ReadDeviceInfo", "Read device information", "读", "Device Info", false, "", QVariant()});
    m_commands.append({0x02, 0x00, 0x01, "ReadMachineStatus", "Read machine status", "读", "Machine Status", false, "", QVariant()});
    m_commands.append({0x03, 0x00, 0x01, "ReadCoordinates", "Read coordinates", "读", "Coordinates", false, "", QVariant()});
    m_commands.append({0x04, 0x00, 0x01, "ReadPLCStatus", "Read PLC status", "读", "PLC Status", false, "", QVariant()});
    m_commands.append({0x05, 0x00, 0x01, "ReadAlarmInfo", "Read alarm information", "读", "Alarm Info", false, "", QVariant()});
    m_commands.append({0x06, 0x00, 0x01, "ReadParam", "Read parameter", "读", "Parameters", true, "param_addr", QVariant()});
    m_commands.append({0x07, 0x00, 0x01, "ReadDiagnose", "Read diagnose information", "读", "Diagnose", false, "", QVariant()});
    m_commands.append({0x10, 0x00, 0x01, "ReadDataBlock", "Read data block", "读", "Data Block", true, "block_addr,length", QVariant()});

    // Write commands
    m_commands.append({0x11, 0x00, 0x02, "WriteParam", "Write parameter", "写", "Parameters", true, "param_addr,value", QVariant()});
    m_commands.append({0x20, 0x00, 0x02, "ControlCommand", "Send control command", "写", "Control", true, "cmd_code", QVariant()});
    m_commands.append({0x21, 0x00, 0x02, "WritePLC", "Write PLC data", "写", "PLC", true, "plc_addr,data", QVariant()});
    m_commands.append({0x22, 0x00, 0x02, "WriteDataBlock", "Write data block", "写", "Data Block", true, "block_addr,data", QVariant()});

    // Control commands
    m_commands.append({0x30, 0x00, 0x03, "Start", "Start machine", "控制", "Machine Control", false, "", QVariant()});
    m_commands.append({0x31, 0x00, 0x03, "Stop", "Stop machine", "控制", "Machine Control", false, "", QVariant()});
    m_commands.append({0x32, 0x00, 0x03, "FeedHold", "Feed hold", "控制", "Machine Control", false, "", QVariant()});
    m_commands.append({0x33, 0x00, 0x03, "Reset", "Reset system", "控制", "Machine Control", false, "", QVariant()});
    m_commands.append({0x34, 0x00, 0x03, "EmergencyStop", "Emergency stop", "控制", "Machine Control", false, "", QVariant()});

    // Data block commands
    m_commands.append({0x50, 0x00, 0x04, "ReadBlock", "Read block", "读", "Data Block", true, "block_num", QVariant()});
    m_commands.append({0x51, 0x00, 0x04, "WriteBlock", "Write block", "写", "Data Block", true, "block_num,data", QVariant()});
}

CommandInfo CommandRegistry::getCommand(quint8 cmd, quint8 sub, quint8 type) const
{
    for (const CommandInfo &info : m_commands) {
        if (info.cmd == cmd && info.sub == sub && info.type == type) {
            return info;
        }
    }
    return CommandInfo();
}

QVector<CommandInfo> CommandRegistry::filterByCategory(const QString &category) const
{
    QVector<CommandInfo> result;
    for (const CommandInfo &info : m_commands) {
        if (info.category == category) {
            result.append(info);
        }
    }
    return result;
}

QVector<CommandInfo> CommandRegistry::filterByFunction(const QString &function) const
{
    QVector<CommandInfo> result;
    for (const CommandInfo &info : m_commands) {
        if (info.function == function) {
            result.append(info);
        }
    }
    return result;
}

QVector<CommandInfo> CommandRegistry::search(const QString &keyword) const
{
    QVector<CommandInfo> result;
    QString lowerKeyword = keyword.toLower();
    for (const CommandInfo &info : m_commands) {
        if (info.name.toLower().contains(lowerKeyword) ||
            info.description.toLower().contains(lowerKeyword)) {
            result.append(info);
            continue;
        }
        // Search by hex command code (e.g., "0x01" or "01")
        if (keyword.toLower().startsWith("0x")) {
            bool ok;
            quint8 cmdCode = keyword.toLower().mid(2).toUInt(&ok, 16);
            if (ok && info.cmd == cmdCode) {
                result.append(info);
            }
        } else if (keyword.length() <= 2) {
            bool ok;
            quint8 cmdCode = keyword.toUInt(&ok, 16);
            if (ok && info.cmd == cmdCode) {
                result.append(info);
            }
        }
    }
    return result;
}