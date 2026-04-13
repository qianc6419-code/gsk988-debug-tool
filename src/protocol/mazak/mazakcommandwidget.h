#ifndef MAZAKCOMMANDWIDGET_H
#define MAZAKCOMMANDWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>

class MazakCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MazakCommandWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    QWidget* createParamTab(const QString& name, int type);
    QWidget* createToolTab();
    QWidget* createCoordTab();
    QWidget* createOperationTab();

    void doRead(int type, int start, int count);
    void doWrite(int type, int start, int count);

    QTabWidget* m_tabs;

    // 通用参数 Tab 组件
    QSpinBox* m_paramStartSpin = nullptr;
    QSpinBox* m_paramCountSpin = nullptr;
    QTableWidget* m_paramTable = nullptr;
    QLabel* m_paramStatusLabel = nullptr;
    int m_currentParamType = 0;

    QComboBox* m_cmnVarTypeCombo = nullptr;
    QSpinBox* m_cmnVarStartSpin = nullptr;
    QSpinBox* m_cmnVarCountSpin = nullptr;
    QTableWidget* m_cmnVarTable = nullptr;
    QLabel* m_cmnVarStatusLabel = nullptr;

    QSpinBox* m_rRegStartSpin = nullptr;
    QSpinBox* m_rRegCountSpin = nullptr;
    QTableWidget* m_rRegTable = nullptr;
    QLabel* m_rRegStatusLabel = nullptr;

    QSpinBox* m_coordStartSpin = nullptr;
    QSpinBox* m_coordCountSpin = nullptr;
    QTableWidget* m_coordTable = nullptr;
    QLabel* m_coordStatusLabel = nullptr;

    QSpinBox* m_toolOffStartSpin = nullptr;
    QSpinBox* m_toolOffCountSpin = nullptr;
    QTableWidget* m_toolOffTable = nullptr;
    QLabel* m_toolOffStatusLabel = nullptr;
};

#endif // MAZAKCOMMANDWIDGET_H
