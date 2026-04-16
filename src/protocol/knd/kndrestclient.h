#ifndef KNDRESTCLIENT_H
#define KNDRESTCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>

class KndRestClient : public QObject
{
    Q_OBJECT
public:
    static KndRestClient& instance();

    void setBaseUrl(const QString& ip);
    bool isConnected() const;
    void setConnected(bool connected);
    void setMockMode(bool mock);
    bool isMockMode() const;

    // 连接健康检查
    bool healthCheck();

    // === 读取方法 (GET) ===
    void getSystemInfo();
    void getStatus();
    void getAlarms();
    void getCoordinates();
    void getAxisLoad();
    void getCycleTime();
    void getWorkCounts();
    void getWorkCountGoals();
    void getOverrideFeed();
    void getOverrideRapid();
    void getOverrideSpindle(int spNo);
    void getSpindleSpeeds();
    void getFeedRate();
    void getWorkCoordinates();
    void getMacros(const QString& names);
    void getGModals();
    void getToolOffsetCount();
    void getToolGeomBatch(int start, int count);
    void getToolWearBatch(int start, int count);
    void getPrograms();
    void getProgram(int no);
    void getProgramExecStatus();
    void getPlcRegion(const QString& region, int offset, const QString& type, int count);

    // === 写入方法 (PUT/POST/DELETE) ===
    void setWorkCoordinate(const QString& name, const QJsonObject& values);
    void setMacro(const QString& varNo, double value);
    void setToolGeom(int ofsNo, const QJsonObject& values);
    void setToolWear(int ofsNo, const QJsonObject& values);
    void setWorkCountTotal(int count);
    void setWorkCountGoal(int count);
    void setCurrentProgram(int no);
    void createProgram(const QString& content);
    void writeProgram(int no, const QString& content);
    void deleteProgram(int no);
    void sendMdi(const QString& content);
    void setPlcRegion(const QString& region, int offset, const QString& type, const QJsonArray& data);
    void clearRelativeCoors();
    void clearCycleTime();

signals:
    void systemInfoReceived(const QJsonObject& data);
    void statusReceived(const QJsonObject& data);
    void alarmsReceived(const QJsonObject& data);
    void coordinatesReceived(const QJsonObject& data);
    void axisLoadReceived(const QJsonObject& data);
    void cycleTimeReceived(const QJsonObject& data);
    void workCountsReceived(const QJsonObject& data);
    void workCountGoalsReceived(const QJsonObject& data);
    void overrideReceived(const QString& type, const QJsonObject& data);
    void spindleSpeedsReceived(const QJsonObject& data);
    void feedRateReceived(const QJsonObject& data);
    void workCoordinatesReceived(const QJsonObject& data);
    void macrosReceived(const QJsonObject& data);
    void gmodalsReceived(const QJsonObject& data);
    void toolOffsetCountReceived(const QJsonObject& data);
    void toolGeomBatchReceived(const QJsonArray& data);
    void toolWearBatchReceived(const QJsonArray& data);
    void programsReceived(const QJsonArray& data);
    void programContentReceived(int no, const QString& content);
    void writeResultReceived(const QString& operation, bool success, const QString& msg);
    void errorOccurred(const QString& msg);

private:
    explicit KndRestClient(QObject* parent = nullptr);

    void sendGet(const QString& path,
                 std::function<void(const QJsonObject&)> handler);
    void sendGetArray(const QString& path,
                      std::function<void(const QJsonArray&)> handler);
    void sendPut(const QString& path, const QJsonObject& body,
                 const QString& operation);
    void sendPutRaw(const QString& path, const QString& body,
                    std::function<void(int, const QJsonObject&)> handler);
    void sendPost(const QString& path, const QString& body,
                  const QString& operation);
    void sendDelete(const QString& path, const QString& operation);

    QNetworkAccessManager* m_nam;
    QString m_baseUrl;
    bool m_connected = false;
    bool m_mockMode = false;
};

#endif // KNDRESTCLIENT_H