#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "itransport.h"
#include <QTcpSocket>
#include <QLineEdit>
#include <QSpinBox>

class TcpTransport : public ITransport {
    Q_OBJECT
public:
    explicit TcpTransport(QObject* parent = nullptr);
    ~TcpTransport();

    QString name() const override { return "TCP"; }
    QWidget* createConfigWidget() override;
    void connectToHost() override;
    void disconnectFromHost() override;
    bool isConnected() const override;
    void send(const QByteArray& data) override;

private slots:
    void onReadyRead();

private:
    QTcpSocket* m_socket;
    QLineEdit* m_ipEdit;
    QSpinBox* m_portSpin;
};

#endif // TCPTRANSPORT_H
