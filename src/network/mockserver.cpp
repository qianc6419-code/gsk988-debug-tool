#include "mockserver.h"
#include "../protocol/framebuilder.h"
#include <QHostAddress>
#include <QDebug>
#include <QtMath>

MockServer::MockServer(QObject *parent)
    : QObject(parent)
    , m_clientSocket(nullptr)
    , m_port(0)
{
    initDefaultResponses();
}

MockServer::~MockServer()
{
    stop();
}

void MockServer::initDefaultResponses()
{
    // "01-00-00" = 16 null bytes + "\x07\xE5\x01\x00" (2025 v1.0)
    QByteArray verData(16, 0x00);
    verData.append("\x07\xE5\x01\x00", 4);
    m_defaultResponses["01-00-00"] = verData;

    // "02-00-00" = "\x01\x00\x64\x64\x88\x13\x00\x01" (running state)
    m_defaultResponses["02-00-00"] = QByteArray("\x01\x00\x64\x64\x88\x13\x00\x01", 8);

    // "03-00-00" = 6 float32 values (123.456, 45.789, 0, 100, 40, 0)
    QByteArray coordData;
    // 123.456f in little-endian IEEE 754
    coordData.append("\x5D\xD8\xED\x42", 4);
    // 45.789f in little-endian IEEE 754
    coordData.append("\x85\xEB\x35\x42", 4);
    // 0.0f
    coordData.append("\x00\x00\x00\x00", 4);
    // 100.0f in little-endian IEEE 754
    coordData.append("\x00\xC8\x42", 3);
    coordData.append("\x00", 1); // padding
    // 40.0f in little-endian IEEE 754
    coordData.append("\x00\x20\x42", 3);
    coordData.append("\x00", 1); // padding
    // 0.0f
    coordData.append("\x00\x00\x00\x00", 4);
    m_defaultResponses["03-00-00"] = coordData;

    // "05-00-00" = "\x00" (no alarms)
    m_defaultResponses["05-00-00"] = QByteArray("\x00", 1);
}

bool MockServer::start(quint16 port)
{
    m_port = port;
    if (!m_server.listen(QHostAddress::Any, port)) {
        emit serverError(m_server.errorString());
        return false;
    }
    connect(&m_server, &QTcpServer::newConnection, this, &MockServer::onNewConnection);
    return true;
}

void MockServer::stop()
{
    if (m_clientSocket) {
        m_clientSocket->disconnectFromConnection();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }
    if (m_server.isListening()) {
        m_server.close();
    }
}

void MockServer::onNewConnection()
{
    m_clientSocket = m_server.nextPendingConnection();
    if (m_clientSocket) {
        QString address = m_clientSocket->peerAddress().toString();
        emit clientConnected(address);
        connect(m_clientSocket, &QTcpSocket::readyRead, this, &MockServer::onClientReadyRead);
        connect(m_clientSocket, &QTcpSocket::disconnected, this, &MockServer::onClientDisconnected);
    }
}

void MockServer::onClientReadyRead()
{
    if (!m_clientSocket) return;

    QByteArray receivedData = m_clientSocket->readAll();
    emit mockDataReceived(receivedData);

    // Parse the frame to extract cmd, sub, type
    quint8 cmd = 0, sub = 0, type = 0;
    QByteArray inData;
    QString error;

    if (FrameBuilder::parseFrame(receivedData, cmd, sub, type, inData, error)) {
        QString key = QString("%1-%2-%3").arg(cmd, 2, 16, QChar('0'))
                                 .arg(sub, 2, 16, QChar('0'))
                                 .arg(type, 2, 16, QChar('0'));

        // Check for custom mock response first, then default
        QByteArray responseData;
        if (m_mockResponses.contains(key)) {
            responseData = m_mockResponses.value(key);
        } else if (m_defaultResponses.contains(key)) {
            responseData = generateDefaultResponse(cmd, sub, type);
        } else {
            // No response for unknown commands - just echo back
            responseData = inData;
        }

        QByteArray responseFrame = buildMockFrame(cmd, sub, type, responseData);
        m_clientSocket->write(responseFrame);
        emit mockDataSent(responseFrame);
    }
}

void MockServer::onClientDisconnected()
{
    emit clientDisconnected();
    if (m_clientSocket) {
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }
}

QByteArray MockServer::generateDefaultResponse(quint8 cmd, quint8 sub, quint8 type)
{
    QString key = QString("%1-%2-%3").arg(cmd, 2, 16, QChar('0'))
                             .arg(sub, 2, 16, QChar('0'))
                             .arg(type, 2, 16, QChar('0'));

    QByteArray baseData = m_defaultResponses.value(key);

    // Add random noise only for coordinate response (cmd 03)
    if (key == "03-00-00" && baseData.size() >= 24) {
        QByteArray noisyData;
        for (int i = 0; i < 6; ++i) {
            float value;
            // Copy 4 bytes for this float
            QByteArray floatBytes = baseData.mid(i * 4, 4);
            // Reinterpret as float (assuming little-endian IEEE 754)
            value = *reinterpret_cast<const float*>(floatBytes.constData());
            // Add ±0.01 random noise
            float noise = (QRandomGenerator::global()->bounded(200) - 100) / 10000.0f;
            value += noise;
            // Convert back to bytes
            noisyData.append(reinterpret_cast<const char*>(&value), 4);
        }
        return noisyData;
    }

    return baseData;
}

QByteArray MockServer::buildMockFrame(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data)
{
    return FrameBuilder::buildFrame(cmd, sub, type, data);
}

void MockServer::setMockResponse(quint8 cmd, quint8 sub, quint8 type, const QByteArray &data)
{
    QString key = QString("%1-%2-%3").arg(cmd, 2, 16, QChar('0'))
                             .arg(sub, 2, 16, QChar('0'))
                             .arg(type, 2, 16, QChar('0'));
    m_mockResponses[key] = data;
}

void MockServer::resetMockResponse(quint8 cmd, quint8 sub, quint8 type)
{
    QString key = QString("%1-%2-%3").arg(cmd, 2, 16, QChar('0'))
                             .arg(sub, 2, 16, QChar('0'))
                             .arg(type, 2, 16, QChar('0'));
    m_mockResponses.remove(key);
}

void MockServer::resetAllResponses()
{
    m_mockResponses.clear();
}
