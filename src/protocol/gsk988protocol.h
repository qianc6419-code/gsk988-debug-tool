#ifndef GSK988PROTOCOL_H
#define GSK988PROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVector>

struct CommandDef {
    quint8 cmdCode;
    QString name;
    QString category;
    QString requestParams;
    QString responseDesc;
};

struct ParsedResponse {
    quint8 cmdCode = 0;
    quint16 errorCode = 0;
    QByteArray rawData;
    bool isValid = false;
    QString errorString;
};

class Gsk988Protocol : public QObject
{
    Q_OBJECT
public:
    explicit Gsk988Protocol(QObject* parent = nullptr);

    QByteArray buildRequest(quint8 cmdCode, const QByteArray& params = {});
    ParsedResponse parseResponse(const QByteArray& frame);

    static QVector<CommandDef> allCommands();
    static CommandDef commandDef(quint8 cmdCode);

    static QString interpretData(quint8 cmdCode, const QByteArray& data);
    static QByteArray mockResponseData(quint8 cmdCode, const QByteArray& requestData);
};

#endif // GSK988PROTOCOL_H
