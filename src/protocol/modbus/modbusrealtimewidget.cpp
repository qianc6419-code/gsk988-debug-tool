#include "modbusrealtimewidget.h"
#include "modbusframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <cstring>

ModbusRealtimeWidget::ModbusRealtimeWidget(QWidget* parent)
    : QWidget(parent)
    , m_pollIndex(0)
    , m_cycleActive(false)
{
    setupUI();

    m_cycleDelayTimer = new QTimer(this);
    m_cycleDelayTimer->setSingleShot(true);
    connect(m_cycleDelayTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRefreshCheck->isChecked())
            startCycle();
    });

    connect(m_manualRefreshBtn, &QPushButton::clicked, this, [this]() {
        startCycle();
    });

    connect(m_autoRefreshCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_intervalCombo->setEnabled(checked);
        if (!checked)
            m_cycleDelayTimer->stop();
    });
}

void ModbusRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // === Poll configuration ===
    auto* configGroup = new QGroupBox("轮询配置");
    auto* configLayout = new QVBoxLayout(configGroup);

    // Add poll item row
    auto* addRow = new QHBoxLayout;
    addRow->addWidget(new QLabel("地址:"));
    m_addrSpin = new QSpinBox;
    m_addrSpin->setRange(0, 65535);
    m_addrSpin->setDisplayIntegerBase(16);
    m_addrSpin->setPrefix("0x");
    addRow->addWidget(m_addrSpin);

    addRow->addWidget(new QLabel("功能码:"));
    m_funcCombo = new QComboBox;
    m_funcCombo->addItem("03 - Read Holding Registers", 0x03);
    m_funcCombo->addItem("04 - Read Input Registers", 0x04);
    m_funcCombo->addItem("01 - Read Coils", 0x01);
    m_funcCombo->addItem("02 - Read Discrete Inputs", 0x02);
    addRow->addWidget(m_funcCombo);

    addRow->addWidget(new QLabel("数量:"));
    m_qtySpin = new QSpinBox;
    m_qtySpin->setRange(1, 125);
    m_qtySpin->setValue(10);
    addRow->addWidget(m_qtySpin);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("名称(可选)");
    m_nameEdit->setMaximumWidth(120);
    addRow->addWidget(m_nameEdit);

    m_dataTypeCombo = new QComboBox;
    m_dataTypeCombo->addItem("UINT16", static_cast<int>(UINT16));
    m_dataTypeCombo->addItem("INT16", static_cast<int>(INT16));
    m_dataTypeCombo->addItem("UINT32", static_cast<int>(UINT32));
    m_dataTypeCombo->addItem("INT32", static_cast<int>(INT32));
    m_dataTypeCombo->addItem("FLOAT32", static_cast<int>(FLOAT32));
    m_dataTypeCombo->addItem("BOOL", static_cast<int>(BOOL));
    addRow->addWidget(m_dataTypeCombo);

    m_addBtn = new QPushButton("+添加");
    m_addBtn->setFixedWidth(60);
    connect(m_addBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::addPollItem);
    addRow->addWidget(m_addBtn);

    m_removeBtn = new QPushButton("-删除");
    m_removeBtn->setFixedWidth(60);
    connect(m_removeBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::removeSelectedPollItem);
    addRow->addWidget(m_removeBtn);

    configLayout->addLayout(addRow);

    // Poll items table
    m_pollTable = new QTableWidget;
    m_pollTable->setColumnCount(5);
    m_pollTable->setHorizontalHeaderLabels({"地址", "功能码", "数量", "名称", "数据类型"});
    m_pollTable->horizontalHeader()->setStretchLastSection(true);
    m_pollTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pollTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pollTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pollTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_pollTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_pollTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pollTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pollTable->setMaximumHeight(120);
    configLayout->addWidget(m_pollTable);

    // Refresh controls
    auto* refreshBar = new QHBoxLayout;
    m_autoRefreshCheck = new QCheckBox("自动刷新");
    m_autoRefreshCheck->setChecked(true);
    refreshBar->addWidget(m_autoRefreshCheck);

    refreshBar->addWidget(new QLabel("间隔:"));
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItem("1 秒", 1);
    m_intervalCombo->addItem("2 秒", 2);
    m_intervalCombo->addItem("5 秒", 5);
    m_intervalCombo->addItem("10 秒", 10);
    m_intervalCombo->addItem("30 秒", 30);
    m_intervalCombo->setCurrentIndex(0);
    refreshBar->addWidget(m_intervalCombo);

    m_manualRefreshBtn = new QPushButton("手动刷新");
    m_manualRefreshBtn->setFixedWidth(80);
    refreshBar->addWidget(m_manualRefreshBtn);
    refreshBar->addStretch();

    configLayout->addLayout(refreshBar);
    mainLayout->addWidget(configGroup);

    // === Results table ===
    auto* resultGroup = new QGroupBox("轮询结果");
    auto* resultLayout = new QVBoxLayout(resultGroup);

    m_resultTable = new QTableWidget;
    m_resultTable->setColumnCount(4);
    m_resultTable->setHorizontalHeaderLabels({"地址", "类型", "原始值", "解析值"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultLayout->addWidget(m_resultTable);

    mainLayout->addWidget(resultGroup, 1);

    // === Hex display ===
    m_hexDisplay = new QTextEdit;
    m_hexDisplay->setReadOnly(true);
    m_hexDisplay->setFontFamily("Consolas");
    m_hexDisplay->setMaximumHeight(80);
    m_hexDisplay->setStyleSheet("QTextEdit { font-size: 11px; background: #1e1e1e; color: #d4d4d4; }");
    mainLayout->addWidget(m_hexDisplay);
}

void ModbusRealtimeWidget::addPollItem()
{
    PollItem item;
    item.startAddr = static_cast<quint16>(m_addrSpin->value());
    item.funcCode = static_cast<quint8>(m_funcCombo->currentData().toInt());
    item.quantity = static_cast<quint16>(m_qtySpin->value());
    item.name = m_nameEdit->text().isEmpty()
        ? QString("Reg_%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()
        : m_nameEdit->text();
    item.dataType = static_cast<DataType>(m_dataTypeCombo->currentData().toInt());

    m_pollItems.append(item);

    int row = m_pollTable->rowCount();
    m_pollTable->insertRow(row);
    m_pollTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
    m_pollTable->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(item.funcCode, 2, 16, QChar('0')).toUpper()));
    m_pollTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.quantity)));
    m_pollTable->setItem(row, 3, new QTableWidgetItem(item.name));
    QStringList typeNames = {"UINT16", "INT16", "UINT32", "INT32", "FLOAT32", "BOOL"};
    m_pollTable->setItem(row, 4, new QTableWidgetItem(typeNames[item.dataType]));

    m_pollTable->selectRow(row);
}

void ModbusRealtimeWidget::removeSelectedPollItem()
{
    int row = m_pollTable->currentRow();
    if (row < 0) return;
    m_pollTable->removeRow(row);
    m_pollItems.remove(row);
}

void ModbusRealtimeWidget::startCycle()
{
    if (m_cycleActive || m_pollItems.isEmpty()) return;
    m_cycleActive = true;
    m_pollIndex = 0;

    const auto& item = m_pollItems[0];
    QByteArray params;
    params.append('\x01'); // unitId = 1
    params.append(ModbusFrameBuilder::encodeAddress(item.startAddr));
    params.append(ModbusFrameBuilder::encodeQuantity(item.quantity));
    emit pollRequest(item.funcCode, params);
}

void ModbusRealtimeWidget::updateData(const ParsedResponse& resp)
{
    // Advance cycle
    if (m_cycleActive) {
        m_pollIndex++;
        if (m_pollIndex >= m_pollItems.size()) {
            m_cycleActive = false;
            if (m_autoRefreshCheck->isChecked()) {
                int secs = m_intervalCombo->currentData().toInt();
                m_cycleDelayTimer->start(secs * 1000);
            }
        } else {
            const auto& item = m_pollItems[m_pollIndex];
            QByteArray params;
            params.append('\x01'); // unitId = 1
            params.append(ModbusFrameBuilder::encodeAddress(item.startAddr));
            params.append(ModbusFrameBuilder::encodeQuantity(item.quantity));
            emit pollRequest(item.funcCode, params);
        }
    }

    // Update results table
    if (m_pollItems.isEmpty() || m_pollIndex == 0) return;
    int idx = m_pollIndex - 1;
    if (idx < 0 || idx >= m_pollItems.size()) return;
    const auto& item = m_pollItems[idx];

    // Ensure result table has enough rows
    while (m_resultTable->rowCount() < idx + 1)
        m_resultTable->insertRow(m_resultTable->rowCount());

    int row = idx;
    QStringList typeNames = {"UINT16", "INT16", "UINT32", "INT32", "FLOAT32", "BOOL"};

    if (!resp.isValid) {
        m_resultTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
        m_resultTable->setItem(row, 1, new QTableWidgetItem(typeNames[item.dataType]));
        m_resultTable->setItem(row, 2, new QTableWidgetItem("--"));
        m_resultTable->setItem(row, 3, new QTableWidgetItem("<span style='color:red;'>错误</span>"));
        return;
    }

    // Parse response data
    if (resp.rawData.size() < 1) return;
    int byteCount = static_cast<quint8>(resp.rawData[0]);

    // Show hex of raw data
    QStringList hexVals;
    for (int i = 1; i <= byteCount && i < resp.rawData.size(); ++i)
        hexVals << QString("%1").arg(static_cast<quint8>(resp.rawData[i]), 2, 16, QChar('0')).toUpper();

    // Decode values
    QStringList decodedVals;
    int dataOffset = 1;

    switch (item.dataType) {
    case UINT16: {
        auto regs = ModbusFrameBuilder::decodeRegisters(resp.rawData, dataOffset, item.quantity);
        for (quint16 v : regs) decodedVals << QString::number(v);
        break;
    }
    case INT16: {
        auto regs = ModbusFrameBuilder::decodeRegisters(resp.rawData, dataOffset, item.quantity);
        for (quint16 v : regs) decodedVals << QString::number(static_cast<qint16>(v));
        break;
    }
    case UINT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            quint32 v = ModbusFrameBuilder::decodeUInt32(resp.rawData, dataOffset + i * 4);
            decodedVals << QString::number(v);
        }
        break;
    }
    case INT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            qint32 v = static_cast<qint32>(ModbusFrameBuilder::decodeUInt32(resp.rawData, dataOffset + i * 4));
            decodedVals << QString::number(v);
        }
        break;
    }
    case FLOAT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            float f;
            quint32 raw = ModbusFrameBuilder::decodeUInt32(resp.rawData, dataOffset + i * 4);
            std::memcpy(&f, &raw, 4);
            decodedVals << QString::number(f, 'f', 2);
        }
        break;
    }
    case BOOL: {
        auto coils = ModbusFrameBuilder::decodeCoils(resp.rawData, dataOffset, item.quantity);
        for (bool b : coils) decodedVals << (b ? "1" : "0");
        break;
    }
    }

    m_resultTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(typeNames[item.dataType]));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(hexVals.join(" ")));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(decodedVals.join(", ")));
}

void ModbusRealtimeWidget::appendHexDisplay(const QByteArray& data, bool isSend)
{
    QStringList hex;
    for (int i = 0; i < data.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();

    QString prefix = isSend ? "[TX]" : "[RX]";
    m_hexDisplay->append(QString("%1 %2").arg(prefix, hex.join(" ")));
}

void ModbusRealtimeWidget::startPolling()
{
    m_pollIndex = 0;
    m_cycleActive = false;
    m_cycleDelayTimer->stop();
    m_resultTable->setRowCount(0);
    startCycle();
}

void ModbusRealtimeWidget::stopPolling()
{
    m_cycleDelayTimer->stop();
    m_cycleActive = false;
}

void ModbusRealtimeWidget::clearData()
{
    m_resultTable->setRowCount(0);
    m_hexDisplay->clear();
}
