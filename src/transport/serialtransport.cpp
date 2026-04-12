#include "serialtransport.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSerialPortInfo>

SerialTransport::SerialTransport(QObject* parent)
    : ITransport(parent)
    , m_portCombo(nullptr)
    , m_baudCombo(nullptr)
{
}

SerialTransport::~SerialTransport() {}

QWidget* SerialTransport::createConfigWidget()
{
    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QLabel("端口:"));
    m_portCombo = new QComboBox;
    for (const auto& info : QSerialPortInfo::availablePorts())
        m_portCombo->addItem(info.portName());
    if (m_portCombo->count() == 0)
        m_portCombo->addItem("COM1");
    layout->addWidget(m_portCombo);

    layout->addWidget(new QLabel("波特率:"));
    m_baudCombo = new QComboBox;
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200"});
    m_baudCombo->setCurrentText("115200");
    layout->addWidget(m_baudCombo);

    return widget;
}

void SerialTransport::connectToHost()
{
    // Stub: actual serial connection will be implemented later
    m_connected = true;
    emit connected();
}

void SerialTransport::disconnectFromHost()
{
    m_connected = false;
    emit disconnected();
}

bool SerialTransport::isConnected() const { return m_connected; }

void SerialTransport::send(const QByteArray& /*data*/)
{
    // Stub: no-op
}
