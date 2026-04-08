#include "mainwindow.h"
#include "network/tcpclient.h"
#include "network/mockserver.h"
#include "ui/realtimewidget.h"
#include "ui/commandtablewidget.h"
#include "ui/dataparsewidget.h"
#include "ui/logwidget.h"
#include "ui/paramwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tcpClient(nullptr)
    , m_mockServer(nullptr)
    , m_realtimeWidget(nullptr)
    , m_commandTableWidget(nullptr)
    , m_dataParseWidget(nullptr)
    , m_logWidget(nullptr)
    , m_paramWidget(nullptr)
    , m_mainToolBar(nullptr)
    , m_statusLabel(nullptr)
    , m_connectionStatusLabel(nullptr)
{
    setupUi();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    resize(1280, 800);
    setWindowTitle("GSK988 Debug Tool");
}

void MainWindow::createMenus()
{
}

void MainWindow::createToolBars()
{
}

void MainWindow::createDockWidgets()
{
}

void MainWindow::createStatusBar()
{
}

void MainWindow::createConnections()
{
}

void MainWindow::onConnectAction()
{
}

void MainWindow::onDisconnectAction()
{
}

void MainWindow::onStartMockServerAction()
{
}

void MainWindow::onStopMockServerAction()
{
}

void MainWindow::onExitAction()
{
}

void MainWindow::onAboutAction()
{
}

void MainWindow::onSendCommandAction()
{
}

void MainWindow::onClearLogAction()
{
}

void MainWindow::onSaveLogAction()
{
}

void MainWindow::onLoadConfigAction()
{
}

void MainWindow::onSaveConfigAction()
{
}
