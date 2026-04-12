#include "modbusrealtimewidget.h"
#include "modbusframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QList>
#include <cstring>

// ---------- static helper for decoding ----------

// Enum indices: BOOL=0, UINT16=1, INT16=2, UINT32=3, INT32=4, FLOAT32=5,
//               BYTE=6, INT8=7, UINT64=8, INT64=9, DOUBLE=10, STRING=11

static QString decodeValue(const QByteArray& rawData, int offset,
                           int dtype,
                           ModbusFrameBuilder::ByteOrder order,
                           int strLen)
{
    switch (dtype) {
    case 0: { // BOOL
        auto coils = ModbusFrameBuilder::decodeCoils(rawData, offset, 1);
        return coils.isEmpty() ? QString() : (coils[0] ? "1" : "0");
    }
    case 1: // UINT16
        return QString::number(ModbusFrameBuilder::decodeUInt16BO(rawData, offset, order));
    case 2: // INT16
        return QString::number(ModbusFrameBuilder::decodeInt16(rawData, offset, order));
    case 3: // UINT32
        return QString::number(ModbusFrameBuilder::decodeUInt32BO(rawData, offset, order));
    case 4: // INT32
        return QString::number(ModbusFrameBuilder::decodeInt32(rawData, offset, order));
    case 5: // FLOAT32
        return QString::number(ModbusFrameBuilder::decodeFloat32(rawData, offset, order), 'f', 2);
    case 6: // BYTE
        return QString::number(ModbusFrameBuilder::decodeUInt8(rawData, offset));
    case 7: // INT8
        return QString::number(ModbusFrameBuilder::decodeInt8(rawData, offset));
    case 8: // UINT64
        return QString::number(ModbusFrameBuilder::decodeUInt64(rawData, offset, order));
    case 9: // INT64
        return QString::number(ModbusFrameBuilder::decodeInt64(rawData, offset, order));
    case 10: // DOUBLE
        return QString::number(ModbusFrameBuilder::decodeDouble(rawData, offset, order), 'f', 6);
    case 11: { // STRING
        int len = qBound(1, strLen, rawData.size() - offset);
        QString s;
        for (int i = 0; i < len && (offset + i) < rawData.size(); ++i)
            s += QChar(static_cast<quint8>(rawData[offset + i]));
        return s;
    }
    }
    return QString();
}

// ---------- ModbusRealtimeWidget ----------

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

    connect(m_timeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        emit timeoutChanged(val);
    });

    connect(m_singleReadBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::doSingleRead);

    connect(m_singleDataTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        bool isString = (idx == static_cast<int>(STRING));
        m_singleStrLenSpin->setVisible(isString);
    });
}

QStringList ModbusRealtimeWidget::typeNames()
{
    return {"BOOL", "UINT16", "INT16", "UINT32", "INT32", "FLOAT32",
            "BYTE", "INT8", "UINT64", "INT64", "DOUBLE", "STRING"};
}

int ModbusRealtimeWidget::dataTypeSize(DataType dt, int strLen) const
{
    switch (dt) {
    case BOOL:    return 1;
    case UINT16:  return 2;
    case INT16:   return 2;
    case UINT32:  return 4;
    case INT32:   return 4;
    case FLOAT32: return 4;
    case BYTE:    return 1;
    case INT8:    return 1;
    case UINT64:  return 8;
    case INT64:   return 8;
    case DOUBLE:  return 8;
    case STRING:  return qMax(1, strLen);
    }
    return 1;
}

void ModbusRealtimeWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // === Config bar ===
    auto* configBar = new QHBoxLayout;

    configBar->addWidget(new QLabel("站号:"));
    m_unitIdSpin = new QSpinBox;
    m_unitIdSpin->setRange(1, 247);
    m_unitIdSpin->setValue(1);
    configBar->addWidget(m_unitIdSpin);

    configBar->addWidget(new QLabel("字节序:"));
    m_byteOrderCombo = new QComboBox;
    m_byteOrderCombo->addItem("ABCD (Big-Endian)",    static_cast<int>(ModbusFrameBuilder::ABCD));
    m_byteOrderCombo->addItem("BADC (Word-Swap)",     static_cast<int>(ModbusFrameBuilder::BADC));
    m_byteOrderCombo->addItem("CDAB (Byte-Swap)",     static_cast<int>(ModbusFrameBuilder::CDAB));
    m_byteOrderCombo->addItem("DCBA (Little-Endian)", static_cast<int>(ModbusFrameBuilder::DCBA));
    configBar->addWidget(m_byteOrderCombo);

    configBar->addWidget(new QLabel("超时(ms):"));
    m_timeoutSpin = new QSpinBox;
    m_timeoutSpin->setRange(100, 30000);
    m_timeoutSpin->setValue(3000);
    m_timeoutSpin->setSingleStep(500);
    configBar->addWidget(m_timeoutSpin);

    configBar->addStretch();
    mainLayout->addLayout(configBar);

    // === Splitter: left = poll config, right = single read ===
    auto* splitter = new QSplitter(Qt::Horizontal);

    // -- Left panel: poll config --
    auto* pollGroup = new QGroupBox("轮询配置");
    auto* pollLayout = new QVBoxLayout(pollGroup);

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
    m_funcCombo->addItem("01 - 读线圈", 0x01);
    m_funcCombo->addItem("02 - 读离散输入", 0x02);
    m_funcCombo->addItem("03 - 读保持寄存器", 0x03);
    m_funcCombo->addItem("04 - 读输入寄存器", 0x04);
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
    QStringList names = typeNames();
    for (int i = 0; i < names.size(); ++i)
        m_dataTypeCombo->addItem(names[i], i);
    addRow->addWidget(m_dataTypeCombo);

    m_addBtn = new QPushButton("+添加");
    m_addBtn->setFixedWidth(60);
    connect(m_addBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::addPollItem);
    addRow->addWidget(m_addBtn);

    m_removeBtn = new QPushButton("-删除");
    m_removeBtn->setFixedWidth(60);
    connect(m_removeBtn, &QPushButton::clicked, this, &ModbusRealtimeWidget::removeSelectedPollItem);
    addRow->addWidget(m_removeBtn);

    pollLayout->addLayout(addRow);

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
    pollLayout->addWidget(m_pollTable);

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

    pollLayout->addLayout(refreshBar);
    splitter->addWidget(pollGroup);

    // -- Right panel: single read --
    auto* singleGroup = new QGroupBox("单次读取");
    auto* singleLayout = new QVBoxLayout(singleGroup);

    auto* singleForm = new QFormLayout;

    m_singleAddrSpin = new QSpinBox;
    m_singleAddrSpin->setRange(0, 65535);
    m_singleAddrSpin->setDisplayIntegerBase(16);
    m_singleAddrSpin->setPrefix("0x");
    singleForm->addRow("地址:", m_singleAddrSpin);

    m_singleFuncCombo = new QComboBox;
    m_singleFuncCombo->addItem("01 - 读线圈", 0x01);
    m_singleFuncCombo->addItem("02 - 读离散输入", 0x02);
    m_singleFuncCombo->addItem("03 - 读保持寄存器", 0x03);
    m_singleFuncCombo->addItem("04 - 读输入寄存器", 0x04);
    singleForm->addRow("功能码:", m_singleFuncCombo);

    m_singleQtySpin = new QSpinBox;
    m_singleQtySpin->setRange(1, 125);
    m_singleQtySpin->setValue(1);
    singleForm->addRow("数量:", m_singleQtySpin);

    m_singleDataTypeCombo = new QComboBox;
    for (int i = 0; i < names.size(); ++i)
        m_singleDataTypeCombo->addItem(names[i], i);
    singleForm->addRow("数据类型:", m_singleDataTypeCombo);

    m_singleStrLenSpin = new QSpinBox;
    m_singleStrLenSpin->setRange(1, 250);
    m_singleStrLenSpin->setValue(10);
    m_singleStrLenSpin->setVisible(false);
    singleForm->addRow("字符串长度:", m_singleStrLenSpin);

    singleLayout->addLayout(singleForm);

    m_singleReadBtn = new QPushButton("读取");
    singleLayout->addWidget(m_singleReadBtn);

    m_singleResultDisplay = new QTextEdit;
    m_singleResultDisplay->setReadOnly(true);
    m_singleResultDisplay->setMaximumHeight(120);
    singleLayout->addWidget(m_singleResultDisplay, 1);

    splitter->addWidget(singleGroup);
    mainLayout->addWidget(splitter);

    // === Result table ===
    auto* resultGroup = new QGroupBox("轮询结果");
    auto* resultLayout = new QVBoxLayout(resultGroup);

    m_resultTable = new QTableWidget;
    m_resultTable->setColumnCount(5);
    m_resultTable->setHorizontalHeaderLabels({"名称", "地址", "类型", "原始值", "解析值"});
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
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
    item.funcCode  = static_cast<quint8>(m_funcCombo->currentData().toInt());
    item.quantity  = static_cast<quint16>(m_qtySpin->value());
    item.name      = m_nameEdit->text().isEmpty()
        ? QString("Reg_%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()
        : m_nameEdit->text();
    item.dataType  = static_cast<DataType>(m_dataTypeCombo->currentData().toInt());

    m_pollItems.append(item);

    int row = m_pollTable->rowCount();
    m_pollTable->insertRow(row);
    m_pollTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
    m_pollTable->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(item.funcCode, 2, 16, QChar('0')).toUpper()));
    m_pollTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.quantity)));
    m_pollTable->setItem(row, 3, new QTableWidgetItem(item.name));
    QStringList names = typeNames();
    m_pollTable->setItem(row, 4, new QTableWidgetItem(names[item.dataType]));

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
    quint8 unitId = static_cast<quint8>(m_unitIdSpin->value());
    QByteArray params;
    params.append(static_cast<char>(unitId));
    params.append(ModbusFrameBuilder::encodeAddress(item.startAddr));
    params.append(ModbusFrameBuilder::encodeQuantity(item.quantity));
    emit pollRequest(item.funcCode, params);
}

void ModbusRealtimeWidget::doSingleRead()
{
    quint8 unitId   = static_cast<quint8>(m_unitIdSpin->value());
    quint16 addr    = static_cast<quint16>(m_singleAddrSpin->value());
    quint8 funcCode = static_cast<quint8>(m_singleFuncCombo->currentData().toInt());
    quint16 qty     = static_cast<quint16>(m_singleQtySpin->value());

    QByteArray params;
    params.append(static_cast<char>(unitId));
    params.append(ModbusFrameBuilder::encodeAddress(addr));
    params.append(ModbusFrameBuilder::encodeQuantity(qty));
    emit singleReadRequest(funcCode, params);
}

void ModbusRealtimeWidget::showSingleReadResult(const ParsedResponse& resp, const QString& interp)
{
    QString html;
    if (!resp.isValid) {
        html = "<span style='color:red;'>读取失败</span>";
    } else {
        QStringList hexVals;
        for (int i = 0; i < resp.rawData.size(); ++i)
            hexVals << QString("%1").arg(static_cast<quint8>(resp.rawData[i]), 2, 16, QChar('0')).toUpper();
        html = QString("<b>RAW:</b> %1<br><b>解析:</b> %2")
                   .arg(hexVals.join(" "), interp);
    }
    m_singleResultDisplay->setHtml(html);
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
            quint8 unitId = static_cast<quint8>(m_unitIdSpin->value());
            QByteArray params;
            params.append(static_cast<char>(unitId));
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
    QStringList names = typeNames();

    if (!resp.isValid) {
        m_resultTable->setItem(row, 0, new QTableWidgetItem(item.name));
        m_resultTable->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
        m_resultTable->setItem(row, 2, new QTableWidgetItem(names[item.dataType]));
        m_resultTable->setItem(row, 3, new QTableWidgetItem("--"));
        m_resultTable->setItem(row, 4, new QTableWidgetItem("<span style='color:red;'>错误</span>"));
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
    auto order = static_cast<ModbusFrameBuilder::ByteOrder>(m_byteOrderCombo->currentData().toInt());

    switch (item.dataType) {
    case BOOL: {
        auto coils = ModbusFrameBuilder::decodeCoils(resp.rawData, dataOffset, item.quantity);
        for (bool b : coils) decodedVals << (b ? "1" : "0");
        break;
    }
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
            quint32 v = ModbusFrameBuilder::decodeUInt32BO(resp.rawData, dataOffset + i * 4, order);
            decodedVals << QString::number(v);
        }
        break;
    }
    case INT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            qint32 v = ModbusFrameBuilder::decodeInt32(resp.rawData, dataOffset + i * 4, order);
            decodedVals << QString::number(v);
        }
        break;
    }
    case FLOAT32: {
        for (int i = 0; i < item.quantity / 2 && dataOffset + i * 4 + 3 < resp.rawData.size(); ++i) {
            float f = ModbusFrameBuilder::decodeFloat32(resp.rawData, dataOffset + i * 4, order);
            decodedVals << QString::number(f, 'f', 2);
        }
        break;
    }
    case BYTE: {
        for (int i = 0; i < item.quantity && dataOffset + i < resp.rawData.size(); ++i) {
            quint8 v = ModbusFrameBuilder::decodeUInt8(resp.rawData, dataOffset + i);
            decodedVals << QString::number(v);
        }
        break;
    }
    case INT8: {
        for (int i = 0; i < item.quantity && dataOffset + i < resp.rawData.size(); ++i) {
            qint8 v = ModbusFrameBuilder::decodeInt8(resp.rawData, dataOffset + i);
            decodedVals << QString::number(v);
        }
        break;
    }
    case UINT64: {
        for (int i = 0; i < item.quantity / 4 && dataOffset + i * 8 + 7 < resp.rawData.size(); ++i) {
            quint64 v = ModbusFrameBuilder::decodeUInt64(resp.rawData, dataOffset + i * 8, order);
            decodedVals << QString::number(v);
        }
        break;
    }
    case INT64: {
        for (int i = 0; i < item.quantity / 4 && dataOffset + i * 8 + 7 < resp.rawData.size(); ++i) {
            qint64 v = ModbusFrameBuilder::decodeInt64(resp.rawData, dataOffset + i * 8, order);
            decodedVals << QString::number(v);
        }
        break;
    }
    case DOUBLE: {
        for (int i = 0; i < item.quantity / 4 && dataOffset + i * 8 + 7 < resp.rawData.size(); ++i) {
            double d = ModbusFrameBuilder::decodeDouble(resp.rawData, dataOffset + i * 8, order);
            decodedVals << QString::number(d, 'f', 6);
        }
        break;
    }
    case STRING: {
        decodedVals << decodeValue(resp.rawData, dataOffset, STRING, order, item.quantity);
        break;
    }
    }

    m_resultTable->setItem(row, 0, new QTableWidgetItem(item.name));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(item.startAddr, 4, 16, QChar('0')).toUpper()));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(names[item.dataType]));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(hexVals.join(" ")));
    m_resultTable->setItem(row, 4, new QTableWidgetItem(decodedVals.join(", ")));
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
    m_singleResultDisplay->clear();
}
