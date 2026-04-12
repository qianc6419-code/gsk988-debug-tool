#include "tcptransport.h"
#include <QHBoxLayout>
#include <QLabel>

TcpTransport::TcpTransport(QObject* parent)
    : ITransport(parent)
    , m_socket(new QTcpSocket(this))
    , m_ipEdit(nullptr)
    , m_portSpin(nullptr)
{
    connect(m_socket, &QTcpSocket::connected, this, &TcpTransport::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        emit disconnected();
    });

    using ErrorSignal = void (QTcpSocket::*)(QAbstractSocket::SocketError);
    connect(m_socket, static_cast<ErrorSignal>(&QTcpSocket::errorOccurred), this,
            [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket->errorString());
    });

    connect(m_socket, &QTcpSocket::readyRead, this, &TcpTransport::onReadyRead);
}

TcpTransport::~TcpTransport()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->abort();
}

QWidget* TcpTransport::createConfigWidget()
{
    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    layout->addWidget(new QLabel("IP:"));
    m_ipEdit = new QLineEdit("192.168.1.100");
    m_ipEdit->setFixedWidth(120);
    layout->addWidget(m_ipEdit);

    layout->addWidget(new QLabel("端口:"));
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(6000);
    m_portSpin->setFixedWidth(75);
    layout->addWidget(m_portSpin);

    return widget;
}

void TcpTransport::connectToHost()
{
    if (!m_ipEdit || !m_portSpin) return;
    m_socket->connectToHost(m_ipEdit->text().trimmed(),
                            static_cast<quint16>(m_portSpin->value()));
}

void TcpTransport::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

bool TcpTransport::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpTransport::send(const QByteArray& data)
{
    if (!isConnected()) return;
    m_socket->write(data);
}

void TcpTransport::onReadyRead()
{
    emit dataReceived(m_socket->readAll());
}
