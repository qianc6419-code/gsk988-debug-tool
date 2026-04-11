#ifndef MODBUSREALTIMEWIDGET_H
#define MODBUSREALTIMEWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVector>
#include "protocol/iprotocol.h"

class ModbusRealtimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusRealtimeWidget(QWidget* parent = nullptr);

    void updateData(const ParsedResponse& resp);
    void startPolling();
    void stopPolling();
    void clearData();

public slots:
    void appendHexDisplay(const QByteArray& data, bool isSend);

signals:
    void pollRequest(quint8 cmdCode, const QByteArray& params);

private:
    void setupUI();
    void addPollItem();
    void removeSelectedPollItem();
    void startCycle();

    enum DataType { UINT16, INT16, UINT32, INT32, FLOAT32, BOOL };

    struct PollItem {
        quint16 startAddr;
        quint8 funcCode;
        quint16 quantity;
        QString name;
        DataType dataType;
    };

    // Controls
    QSpinBox* m_addrSpin;
    QComboBox* m_funcCombo;
    QSpinBox* m_qtySpin;
    QLineEdit* m_nameEdit;
    QComboBox* m_dataTypeCombo;
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;

    QCheckBox* m_autoRefreshCheck;
    QComboBox* m_intervalCombo;
    QPushButton* m_manualRefreshBtn;

    // Poll list
    QTableWidget* m_pollTable;

    // Results
    QTableWidget* m_resultTable;

    // Hex display
    QTextEdit* m_hexDisplay;

    // Polling state
    QTimer* m_cycleDelayTimer;
    int m_pollIndex;
    bool m_cycleActive;
    QVector<PollItem> m_pollItems;
};

#endif // MODBUSREALTIMEWIDGET_H
