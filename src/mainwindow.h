#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>

class Gsk988Protocol;
class TcpClient;
class MockServer;
class RealtimeWidget;
class CommandWidget;
class ParseWidget;
class LogWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onResponseReceived(const QByteArray& frame);
    void onResponseTimeout();

private:
    void setupToolBar();
    void setupTabs();
    void setupConnections();
    void updateConnectionIndicator(int state);
    void notifyXtuis(const QString& title, const QString& content);

    enum ConnState { Disconnected = 0, Connected = 1, Error = 2 };

    Gsk988Protocol* m_protocol;
    TcpClient* m_client;
    MockServer* m_mockServer;

    QLineEdit* m_ipEdit;
    QSpinBox* m_portSpin;
    QComboBox* m_modeCombo;
    QPushButton* m_connectBtn;
    QLabel* m_statusIndicator;
    QLabel* m_statusLabel;

    QTabWidget* m_tabWidget;
    RealtimeWidget* m_realtimeTab;
    CommandWidget* m_commandTab;
    ParseWidget* m_parseTab;
    LogWidget* m_logTab;

    bool m_waitingManualResponse;
    bool m_needStartPolling;
};

#endif // MAINWINDOW_H
