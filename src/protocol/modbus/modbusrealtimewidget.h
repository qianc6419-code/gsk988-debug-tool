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
    void showSingleReadResult(const ParsedResponse& resp, const QString& interp);

public slots:
    void appendHexDisplay(const QByteArray& data, bool isSend);

signals:
    void pollRequest(quint8 cmdCode, const QByteArray& params);
    void singleReadRequest(quint8 cmdCode, const QByteArray& params);
    void timeoutChanged(int ms);

private:
    void setupUI();
    void addPollItem();
    void removeSelectedPollItem();
    void startCycle();
    void doSingleRead();

    enum DataType {
        BOOL, UINT16, INT16, UINT32, INT32, FLOAT32,
        BYTE, INT8, UINT64, INT64, DOUBLE, STRING
    };

    struct PollItem {
        quint16 startAddr;
        quint8 funcCode;
        quint16 quantity;
        QString name;
        DataType dataType;
    };

    // Global config bar
    QSpinBox* m_unitIdSpin;
    QComboBox* m_byteOrderCombo;
    QSpinBox* m_timeoutSpin;

    // Poll item controls
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

    // Single read panel
    QSpinBox* m_singleAddrSpin;
    QComboBox* m_singleFuncCombo;
    QSpinBox* m_singleQtySpin;
    QComboBox* m_singleDataTypeCombo;
    QSpinBox* m_singleStrLenSpin;
    QPushButton* m_singleReadBtn;
    QTextEdit* m_singleResultDisplay;

    // Results
    QTableWidget* m_resultTable;

    // Hex display
    QTextEdit* m_hexDisplay;

    // Polling state
    QTimer* m_cycleDelayTimer;
    int m_pollIndex;
    bool m_cycleActive;
    QVector<PollItem> m_pollItems;

    // Helpers
    static QStringList typeNames();
    int dataTypeSize(DataType dt, int strLen = 0) const;
};

#endif // MODBUSREALTIMEWIDGET_H
