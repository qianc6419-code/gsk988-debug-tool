#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>
#include <QByteArray>

class ITransport;
class IProtocol;
class IProtocolWidgetFactory;
class MockServer;
class LogWidget;

// Forward declare GSK988-specific widget signals we need
class Gsk988RealtimeWidget;
class Gsk988CommandWidget;
class ModbusRealtimeWidget;
class ModbusCommandWidget;
class FanucRealtimeWidget;
class FanucCommandWidget;
class SiemensRealtimeWidget;
class SiemensCommandWidget;
class SiemensProtocol;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onProtocolChanged();
    void onTransportChanged();
    void onDataReceived(const QByteArray& rawData);
    void onResponseTimeout();

private:
    void setupToolBar();
    void setupTabs();
    void switchProtocol(int index);
    void switchTransport(int index);
    void setupProtocolConnections();
    void updateConnectionIndicator(int state);
    void notifyXtuis(const QString& title, const QString& content);

    enum ConnState { Disconnected = 0, Connected = 1, Error = 2 };

    // Core objects
    ITransport* m_transport = nullptr;
    IProtocol* m_protocol = nullptr;
    IProtocolWidgetFactory* m_widgetFactory = nullptr;
    MockServer* m_mockServer = nullptr;

    // Transport config widget (owned by toolbar layout)
    QWidget* m_transportConfigWidget = nullptr;
    QAction* m_transportConfigAction = nullptr;

    // Toolbar
    QToolBar* m_toolbar = nullptr;

    // Toolbar controls
    QComboBox* m_protocolCombo;
    QComboBox* m_transportCombo;
    QComboBox* m_modeCombo;
    QPushButton* m_connectBtn;
    QLabel* m_statusIndicator;
    QLabel* m_statusLabel;

    // Tabs
    QTabWidget* m_tabWidget;
    QWidget* m_realtimeTab = nullptr;
    QWidget* m_commandTab = nullptr;
    QWidget* m_parseTab = nullptr;
    LogWidget* m_logTab;

    // State (moved from TcpClient)
    QTimer* m_timeoutTimer;
    QByteArray m_buffer;
    bool m_waitingManualResponse = false;
    bool m_needStartPolling = false;
};

#endif // MAINWINDOW_H
