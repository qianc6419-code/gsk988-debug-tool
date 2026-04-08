#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTextEdit>
#include <QLabel>

class TcpClient;
class MockServer;
class RealtimeWidget;
class CommandTableWidget;
class DataParseWidget;
class LogWidget;
class ParamWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectAction();
    void onDisconnectAction();
    void onStartMockServerAction();
    void onStopMockServerAction();
    void onExitAction();
    void onAboutAction();
    void onSendCommandAction();
    void onClearLogAction();
    void onSaveLogAction();
    void onLoadConfigAction();
    void onSaveConfigAction();

private:
    void setupUi();
    void createMenus();
    void createToolBars();
    void createDockWidgets();
    void createStatusBar();
    void createConnections();

    // Network
    TcpClient *m_tcpClient;
    MockServer *m_mockServer;

    // UI Widgets
    RealtimeWidget *m_realtimeWidget;
    CommandTableWidget *m_commandTableWidget;
    DataParseWidget *m_dataParseWidget;
    LogWidget *m_logWidget;
    ParamWidget *m_paramWidget;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_connectionMenu;
    QMenu *m_commandMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;

    // Actions
    QAction *m_connectAction;
    QAction *m_disconnectAction;
    QAction *m_startMockServerAction;
    QAction *m_stopMockServerAction;
    QAction *m_exitAction;
    QAction *m_sendCommandAction;
    QAction *m_clearLogAction;
    QAction *m_saveLogAction;
    QAction *m_loadConfigAction;
    QAction *m_saveConfigAction;
    QAction *m_aboutAction;

    // Toolbars
    QToolBar *m_mainToolBar;

    // Status bar
    QLabel *m_statusLabel;
    QLabel *m_connectionStatusLabel;
};

#endif // MAINWINDOW_H
