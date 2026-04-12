#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "itransport.h"
#include <QComboBox>

class QSerialPort;

class SerialTransport : public ITransport {
    Q_OBJECT
public:
    explicit SerialTransport(QObject* parent = nullptr);
    ~SerialTransport();

    QString name() const override { return "串口"; }
    QWidget* createConfigWidget() override;
    void connectToHost() override;
    void disconnectFromHost() override;
    bool isConnected() const override;
    void send(const QByteArray& data) override;

private:
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    bool m_connected = false;
};

#endif // SERIALTRANSPORT_H
