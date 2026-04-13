#ifndef KNDCOMMANDWIDGET_H
#define KNDCOMMANDWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>

class KndCommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KndCommandWidget(QWidget* parent = nullptr);

private:
    void setupUI();
    QWidget* createWorkCoordTab();
    QWidget* createMacroTab();
    QWidget* createToolTab();
    QWidget* createProgramTab();
    QWidget* createPlcTab();
    QWidget* createWorkCountTab();
    QWidget* createOperationTab();

    QTabWidget* m_tabs;

    QTableWidget* m_coordTable;
    QLabel* m_coordStatusLabel;

    QLineEdit* m_macroNamesEdit;
    QTableWidget* m_macroTable;
    QLabel* m_macroStatusLabel;

    QTabWidget* m_toolTabs;
    QSpinBox* m_toolGeomStartSpin;
    QSpinBox* m_toolGeomCountSpin;
    QTableWidget* m_toolGeomTable;
    QLabel* m_toolGeomStatusLabel;
    QSpinBox* m_toolWearStartSpin;
    QSpinBox* m_toolWearCountSpin;
    QTableWidget* m_toolWearTable;
    QLabel* m_toolWearStatusLabel;

    QListWidget* m_progList;
    QTextEdit* m_progContentEdit;
    QSpinBox* m_progNoSpin;
    QLabel* m_progStatusLabel;

    QComboBox* m_plcRegionCombo;
    QSpinBox* m_plcOffsetSpin;
    QComboBox* m_plcTypeCombo;
    QSpinBox* m_plcCountSpin;
    QTableWidget* m_plcTable;
    QLabel* m_plcStatusLabel;

    QSpinBox* m_countTotalSpin;
    QSpinBox* m_countGoalSpin;
    QLabel* m_countStatusLabel;
};

#endif // KNDCOMMANDWIDGET_H