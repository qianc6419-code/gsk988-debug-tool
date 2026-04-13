#include "protocol/knd/kndrestclient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QUrl>

KndRestClient& KndRestClient::instance()
{
    static KndRestClient inst;
    return inst;
}

KndRestClient::KndRestClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void KndRestClient::setBaseUrl(const QString& ip)
{
    m_baseUrl = QString("http://%1/api/v1.2").arg(ip);
}

bool KndRestClient::isConnected() const { return m_connected; }
void KndRestClient::setConnected(bool connected) { m_connected = connected; }
void KndRestClient::setMockMode(bool mock) { m_mockMode = mock; }
bool KndRestClient::isMockMode() const { return m_mockMode; }

// === 内部 HTTP 方法 ===

void KndRestClient::sendGet(const QString& path,
                             std::function<void(const QJsonObject&)> handler)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            handler(doc.object());
        } else {
            emit errorOccurred("无效的 JSON 响应");
        }
    });
}

void KndRestClient::sendGetArray(const QString& path,
                                  std::function<void(const QJsonArray&)> handler)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isArray()) {
            handler(doc.array());
        } else if (doc.isObject()) {
            QJsonArray arr;
            arr.append(doc.object());
            handler(arr);
        } else {
            emit errorOccurred("无效的 JSON 响应");
        }
    });
}

void KndRestClient::sendPut(const QString& path, const QJsonObject& body,
                             const QString& operation)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(body);
    auto* reply = m_nam->put(req, doc.toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, operation]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit writeResultReceived(operation, false, reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument respDoc = QJsonDocument::fromJson(data);
        if (respDoc.isObject()) {
            QJsonObject resp = respDoc.object();
            if (resp.contains("error-message")) {
                emit writeResultReceived(operation, false, resp["error-message"].toString());
            } else {
                emit writeResultReceived(operation, true, "成功");
            }
        } else {
            emit writeResultReceived(operation, true, "成功");
        }
    });
}

void KndRestClient::sendPutRaw(const QString& path, const QString& body,
                                std::function<void(int, const QJsonObject&)> handler)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

    auto* reply = m_nam->put(req, body.toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        handler(status, doc.isObject() ? doc.object() : QJsonObject());
    });
}

void KndRestClient::sendPost(const QString& path, const QString& body,
                              const QString& operation)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

    auto* reply = m_nam->post(req, body.toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, operation]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit writeResultReceived(operation, false, reply->errorString());
            return;
        }
        emit writeResultReceived(operation, true, "成功");
    });
}

void KndRestClient::sendDelete(const QString& path, const QString& operation)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest req(url);

    auto* reply = m_nam->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, operation]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit writeResultReceived(operation, false, reply->errorString());
            return;
        }
        emit writeResultReceived(operation, true, "成功");
    });
}

// === 读取方法实现 ===

void KndRestClient::getSystemInfo()
{
    sendGet("/", [this](const QJsonObject& data) { emit systemInfoReceived(data); });
}

void KndRestClient::getStatus()
{
    sendGet("/status", [this](const QJsonObject& data) { emit statusReceived(data); });
}

void KndRestClient::getAlarms()
{
    sendGet("/alarms/", [this](const QJsonObject& data) { emit alarmsReceived(data); });
}

void KndRestClient::getCoordinates()
{
    sendGet("/coors/", [this](const QJsonObject& data) { emit coordinatesReceived(data); });
}

void KndRestClient::getAxisLoad()
{
    sendGet("/coors/load", [this](const QJsonObject& data) { emit axisLoadReceived(data); });
}

void KndRestClient::getCycleTime()
{
    sendGet("/cycletime", [this](const QJsonObject& data) { emit cycleTimeReceived(data); });
}

void KndRestClient::getWorkCounts()
{
    sendGet("/workcounts/", [this](const QJsonObject& data) { emit workCountsReceived(data); });
}

void KndRestClient::getWorkCountGoals()
{
    sendGet("/workcountgoals/", [this](const QJsonObject& data) { emit workCountGoalsReceived(data); });
}

void KndRestClient::getOverrideFeed()
{
    sendGet("/overrides/feed", [this](const QJsonObject& data) {
        emit overrideReceived("feed", data);
    });
}

void KndRestClient::getOverrideRapid()
{
    sendGet("/overrides/rapid", [this](const QJsonObject& data) {
        emit overrideReceived("rapid", data);
    });
}

void KndRestClient::getOverrideSpindle(int spNo)
{
    sendGet(QString("/overrides/sp/%1").arg(spNo), [this](const QJsonObject& data) {
        emit overrideReceived("spindle", data);
    });
}

void KndRestClient::getSpindleSpeeds()
{
    sendGet("/sp/speeds/", [this](const QJsonObject& data) { emit spindleSpeedsReceived(data); });
}

void KndRestClient::getFeedRate()
{
    sendGet("/feedback/F", [this](const QJsonObject& data) { emit feedRateReceived(data); });
}

void KndRestClient::getWorkCoordinates()
{
    sendGet("/workcoors/", [this](const QJsonObject& data) { emit workCoordinatesReceived(data); });
}

void KndRestClient::getMacros(const QString& names)
{
    sendGet(QString("/vars/?name=%1").arg(names), [this](const QJsonObject& data) {
        emit macrosReceived(data);
    });
}

void KndRestClient::getGModals()
{
    sendGet("/gmodals/", [this](const QJsonObject& data) { emit gmodalsReceived(data); });
}

void KndRestClient::getToolOffsetCount()
{
    sendGet("/ofs/count", [this](const QJsonObject& data) { emit toolOffsetCountReceived(data); });
}

void KndRestClient::getToolGeomBatch(int start, int count)
{
    sendGetArray(QString("/ofs/geoms/?start=%1&count=%2").arg(start).arg(count),
        [this](const QJsonArray& data) { emit toolGeomBatchReceived(data); });
}

void KndRestClient::getToolWearBatch(int start, int count)
{
    sendGetArray(QString("/ofs/wears/?start=%1&count=%2").arg(start).arg(count),
        [this](const QJsonArray& data) { emit toolWearBatchReceived(data); });
}

void KndRestClient::getPrograms()
{
    sendGetArray("/progs/", [this](const QJsonArray& data) { emit programsReceived(data); });
}

void KndRestClient::getProgram(int no)
{
    QUrl url(m_baseUrl + QString("/progs/%1").arg(no));
    QNetworkRequest req(url);
    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, no]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }
        QString content = QString::fromUtf8(reply->readAll());
        emit programContentReceived(no, content);
    });
}

void KndRestClient::getProgramExecStatus()
{
    sendGet("/progs/exec-status", [this](const QJsonObject& data) {
        emit systemInfoReceived(data);
    });
}

void KndRestClient::getPlcRegion(const QString& region, int offset,
                                  const QString& type, int count)
{
    sendGet(QString("/plc/vm/%1%2?type=%3&count=%4")
                .arg(region).arg(offset).arg(type).arg(count),
        [this](const QJsonObject& data) { emit systemInfoReceived(data); });
}

// === 写入方法实现 ===

void KndRestClient::setWorkCoordinate(const QString& name, const QJsonObject& values)
{
    sendPut(QString("/workcoors/%1").arg(name), values, "设置工件坐标系");
}

void KndRestClient::setMacro(const QString& varNo, double value)
{
    QJsonObject body;
    body["value"] = value;
    sendPut(QString("/vars/%1").arg(varNo), body, "设置宏变量");
}

void KndRestClient::setToolGeom(int ofsNo, const QJsonObject& values)
{
    sendPut(QString("/ofs/geoms/%1").arg(ofsNo), values, "设置刀补形状");
}

void KndRestClient::setToolWear(int ofsNo, const QJsonObject& values)
{
    sendPut(QString("/ofs/wears/%1").arg(ofsNo), values, "设置刀补磨损");
}

void KndRestClient::setWorkCountTotal(int count)
{
    QJsonObject body;
    body["total"] = count;
    sendPut("/workcounts/total", body, "设置加工计数");
}

void KndRestClient::setWorkCountGoal(int count)
{
    QJsonObject body;
    body["total"] = count;
    sendPut("/workcountgoals/total", body, "设置目标件数");
}

void KndRestClient::setCurrentProgram(int no)
{
    QJsonObject body;
    body["no"] = no;
    sendPut("/progs/cur", body, "设置当前程序");
}

void KndRestClient::createProgram(const QString& content)
{
    sendPost("/progs/", content, "创建程序");
}

void KndRestClient::writeProgram(int no, const QString& content)
{
    sendPutRaw(QString("/progs/%1").arg(no), content,
        [this](int, const QJsonObject&) {
            emit writeResultReceived("写入程序", true, "成功");
        });
}

void KndRestClient::deleteProgram(int no)
{
    sendDelete(QString("/progs/%1").arg(no), "删除程序");
}

void KndRestClient::sendMdi(const QString& content)
{
    sendPutRaw("/progs/mdi", content,
        [this](int, const QJsonObject&) {
            emit writeResultReceived("MDI", true, "成功");
        });
}

void KndRestClient::setPlcRegion(const QString& region, int offset,
                                  const QString& type, const QJsonArray& data)
{
    QJsonObject body;
    body["type"] = type;
    body["data"] = data;
    sendPut(QString("/plc/vm/%1%2").arg(region).arg(offset), body, "设置PLC数据");
}

void KndRestClient::clearRelativeCoors()
{
    sendDelete("/coors/relative", "清零相对坐标");
}

void KndRestClient::clearCycleTime()
{
    QJsonObject body;
    body["total"] = 0;
    sendPut("/cycletime", body, "清零加工时间");
}