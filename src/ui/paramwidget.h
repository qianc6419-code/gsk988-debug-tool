#ifndef PARAMWIDGET_H
#define PARAMWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class ParamWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ParamWidget(QWidget *parent = nullptr);
    ~ParamWidget();

    void loadParameters();
    void saveParameters();
    void resetToDefaults();

signals:
    void parametersChanged();

private slots:
    void onParameterChanged(int row, int column);
    void onLoadParams();
    void onSaveParams();
    void onResetParams();

private:
    void setupUi();

    QTableWidget *m_paramTableWidget;
    QPushButton *m_loadButton;
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
};

#endif // PARAMWIDGET_H
