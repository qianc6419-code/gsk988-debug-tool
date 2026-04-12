#include "modbuscommandwidget.h"
#include "modbusframebuilder.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QFormLayout>
#include <QRegularExpression>

// Category colors
static QMap<QString, QString> categoryColors()
{
    return {
        {"基本读写", "#4CAF50"},
        {"诊断状态", "#FF9800"},
        {"文件记录", "#9C27B0"},
        {"其他",     "#607D8B"},
    };
}

ModbusCommandWidget::ModbusCommandWidget(QWidget* parent)
    : QWidget(parent)
    , m_commands(ModbusProtocol::allCommandsRaw())
{
    setupUI();
    populateTable();
}

void ModbusCommandWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // ===== Left: command table =====
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Filter bar
    auto* filterBar = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("搜索功能码/名称...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ModbusCommandWidget::applyFilter);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem("全部类别");
    QSet<QString> categories;
    for (const auto& c : m_commands) categories.insert(c.category);
    for (const auto& cat : categories) m_categoryCombo->addItem(cat);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { applyFilter(m_searchEdit->text()); });

    filterBar->addWidget(m_searchEdit);
    filterBar->addWidget(m_categoryCombo);
    leftLayout->addLayout(filterBar);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"类型", "功能码", "名称"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row) {
        if (row < 0 || row >= m_commands.size()) return;
        // Highlight selected row (skip columns that use cell widgets, no QTableWidgetItem)
        for (int c = 0; c < m_table->columnCount(); ++c) {
            if (auto* it = m_table->item(row, c))
                it->setSelected(true);
        }
    });

    leftLayout->addWidget(m_table);
    mainLayout->addWidget(leftPanel, 2);

    // ===== Right: parameter panel + result =====
    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Unit ID
    auto* unitBar = new QHBoxLayout;
    unitBar->addWidget(new QLabel("从站ID:"));
    m_unitIdSpin = new QSpinBox;
    m_unitIdSpin->setRange(1, 247);
    m_unitIdSpin->setValue(1);
    unitBar->addWidget(m_unitIdSpin);
    unitBar->addStretch();
    rightLayout->addLayout(unitBar);

    // Dynamic parameter panel
    m_paramPanel = new QWidget;
    auto* paramLayout = new QFormLayout(m_paramPanel);
    paramLayout->setContentsMargins(0, 0, 0, 0);

    m_addrEdit = new QLineEdit;
    m_addrEdit->setPlaceholderText("0x0000");
    m_qtyEdit = new QLineEdit;
    m_qtyEdit->setPlaceholderText("1");
    m_writeAddrEdit = new QLineEdit;
    m_writeAddrEdit->setPlaceholderText("0x0000");
    m_writeQtyEdit = new QLineEdit;
    m_writeQtyEdit->setPlaceholderText("1");
    m_valueEdit = new QLineEdit;
    m_valueEdit->setPlaceholderText("HEX值, 如 00 0A 01 72");

    paramLayout->addRow("起始地址:", m_addrEdit);
    paramLayout->addRow("数量:", m_qtyEdit);
    paramLayout->addRow("写地址:", m_writeAddrEdit);
    paramLayout->addRow("写数量:", m_writeQtyEdit);
    paramLayout->addRow("值:", m_valueEdit);

    m_dataTypeCombo = new QComboBox;
    m_dataTypeCombo->addItem("(自动)", -1);
    m_dataTypeCombo->addItem("UINT16", 0);
    m_dataTypeCombo->addItem("INT16", 1);
    m_dataTypeCombo->addItem("UINT32", 2);
    m_dataTypeCombo->addItem("INT32", 3);
    m_dataTypeCombo->addItem("FLOAT32", 4);
    m_dataTypeCombo->addItem("BOOL", 5);
    m_dataTypeCombo->addItem("BYTE", 6);
    m_dataTypeCombo->addItem("INT8", 7);
    m_dataTypeCombo->addItem("UINT64", 8);
    m_dataTypeCombo->addItem("INT64", 9);
    m_dataTypeCombo->addItem("DOUBLE", 10);
    m_dataTypeCombo->addItem("STRING", 11);
    paramLayout->addRow("数据类型:", m_dataTypeCombo);
    m_dataTypeCombo->setVisible(false);

    m_byteOrderCombo = new QComboBox;
    m_byteOrderCombo->addItem("AB CD (大端)", 0);
    m_byteOrderCombo->addItem("BA DC", 1);
    m_byteOrderCombo->addItem("CD AB", 2);
    m_byteOrderCombo->addItem("DC BA (小端)", 3);
    paramLayout->addRow("字节序:", m_byteOrderCombo);
    m_byteOrderCombo->setVisible(false);

    rightLayout->addWidget(m_paramPanel);

    // Send button
    auto* sendBtn = new QPushButton("发送");
    sendBtn->setFixedHeight(32);
    connect(sendBtn, &QPushButton::clicked, this, [this]() {
        int row = m_table->currentRow();
        if (row < 0) return;

        // Find actual command index (table may be filtered)
        int cmdIdx = -1;
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (i == row) {
                // Get the command code from the table item
                auto* item = m_table->item(i, 1);
                if (!item) return;
                QString codeStr = item->text();
                // Find matching command
                for (int j = 0; j < m_commands.size(); ++j) {
                    if (QString("0x%1").arg(m_commands[j].cmdCode, 2, 16, QChar('0')).toUpper() == codeStr) {
                        cmdIdx = j;
                        break;
                    }
                }
                break;
            }
        }
        if (cmdIdx < 0) return;

        const auto& cmd = m_commands[cmdIdx];
        quint8 funcCode = cmd.cmdCode;
        quint8 unitId = static_cast<quint8>(m_unitIdSpin->value());

        QByteArray params;
        params.append(static_cast<char>(unitId));

        // Build PDU data based on function code
        auto parseHex = [](const QString& text) -> QByteArray {
            QString clean = text;
            clean.remove(' ');
            clean.remove(',');
            clean.remove("0x", Qt::CaseInsensitive);
            return QByteArray::fromHex(clean.toUtf8());
        };

        auto parseAddr = [this]() -> quint16 {
            QString text = m_addrEdit->text().trimmed();
            bool ok;
            if (text.startsWith("0x", Qt::CaseInsensitive))
                return static_cast<quint16>(text.toUShort(&ok, 16));
            return static_cast<quint16>(text.toUShort(&ok, 10));
        };

        auto parseQty = [this]() -> quint16 {
            QString text = m_qtyEdit->text().trimmed();
            bool ok;
            return static_cast<quint16>(text.toUShort(&ok, text.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10));
        };

        switch (funcCode) {
        case 0x01: case 0x02: case 0x03: case 0x04: {
            // Read: address + quantity
            params.append(ModbusFrameBuilder::encodeAddress(parseAddr()));
            params.append(ModbusFrameBuilder::encodeQuantity(parseQty()));
            break;
        }
        case 0x05: {
            // Write Single Coil: address + value (FF00 or 0000)
            params.append(ModbusFrameBuilder::encodeAddress(parseAddr()));
            quint16 val = (m_valueEdit->text().trimmed().toLower() == "on" ||
                          m_valueEdit->text().trimmed() == "1") ? 0xFF00 : 0x0000;
            params.append(ModbusFrameBuilder::encodeValue(val));
            break;
        }
        case 0x06: {
            // Write Single Register: address + value
            params.append(ModbusFrameBuilder::encodeAddress(parseAddr()));
            auto hexData = parseHex(m_valueEdit->text());
            if (hexData.size() >= 2) {
                params.append(hexData.left(2));
            } else {
                bool ok;
                quint16 val = static_cast<quint16>(m_valueEdit->text().toUShort(&ok, 0));
                params.append(ModbusFrameBuilder::encodeValue(val));
            }
            break;
        }
        case 0x0F: {
            // Write Multiple Coils: address + quantity + byteCount + values
            quint16 addr = parseAddr();
            quint16 qty = parseQty();
            auto hexData = parseHex(m_valueEdit->text());
            params.append(ModbusFrameBuilder::encodeAddress(addr));
            params.append(ModbusFrameBuilder::encodeQuantity(qty));
            params.append(static_cast<char>(hexData.size()));
            params.append(hexData);
            break;
        }
        case 0x10: {
            // Write Multiple Registers: address + quantity + byteCount + values
            quint16 addr = parseAddr();
            auto hexData = parseHex(m_valueEdit->text());
            quint16 qty = static_cast<quint16>(hexData.size() / 2);
            params.append(ModbusFrameBuilder::encodeAddress(addr));
            params.append(ModbusFrameBuilder::encodeQuantity(qty));
            params.append(static_cast<char>(hexData.size()));
            params.append(hexData);
            break;
        }
        case 0x17: {
            // Read/Write Multiple Registers
            quint16 readAddr = parseAddr();
            quint16 readQty = parseQty();
            auto writeAddrText = m_writeAddrEdit->text().trimmed();
            quint16 writeAddr = 0;
            if (!writeAddrText.isEmpty()) {
                bool ok;
                writeAddr = static_cast<quint16>(writeAddrText.toUShort(&ok, writeAddrText.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10));
            }
            auto hexData = parseHex(m_valueEdit->text());
            quint16 writeQty = static_cast<quint16>(hexData.size() / 2);
            params.append(ModbusFrameBuilder::encodeAddress(readAddr));
            params.append(ModbusFrameBuilder::encodeQuantity(readQty));
            params.append(ModbusFrameBuilder::encodeAddress(writeAddr));
            params.append(ModbusFrameBuilder::encodeQuantity(writeQty));
            params.append(static_cast<char>(hexData.size()));
            params.append(hexData);
            break;
        }
        case 0x08: {
            // Diagnostics: sub-function + data
            auto hexData = parseHex(m_valueEdit->text());
            if (hexData.size() >= 2)
                params.append(hexData);
            break;
        }
        case 0x2B: {
            // MEI: type + data
            auto hexData = parseHex(m_valueEdit->text());
            params.append(hexData);
            break;
        }
        default:
            // No additional params needed (0x07, 0x0B, 0x0C, 0x11, 0x14, 0x15)
            // For 0x14/0x15, try to append hex data if provided
            if (!m_valueEdit->text().trimmed().isEmpty()) {
                params.append(parseHex(m_valueEdit->text()));
            }
            break;
        }

        emit sendCommand(funcCode, params);
    });
    rightLayout->addWidget(sendBtn);

    // Result display
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");

    rightLayout->addWidget(m_resultDisplay, 1);

    mainLayout->addWidget(rightPanel, 3);

    // Show/hide param fields on table row change
    connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row) {
        if (row < 0) return;
        // Find the command for this row
        auto* codeItem = m_table->item(row, 1);
        if (!codeItem) return;
        QString codeStr = codeItem->text();
        for (const auto& cmd : m_commands) {
            if (QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper() == codeStr) {
                // Update visibility of param fields
                bool needsAddr = (cmd.cmdCode != 0x07 && cmd.cmdCode != 0x0B &&
                                  cmd.cmdCode != 0x0C && cmd.cmdCode != 0x11);
                bool needsQty = (cmd.cmdCode == 0x01 || cmd.cmdCode == 0x02 ||
                                 cmd.cmdCode == 0x03 || cmd.cmdCode == 0x04 ||
                                 cmd.cmdCode == 0x0F || cmd.cmdCode == 0x10);
                bool needsWriteAddr = (cmd.cmdCode == 0x17);
                bool needsValue = (cmd.cmdCode == 0x05 || cmd.cmdCode == 0x06 ||
                                   cmd.cmdCode == 0x0F || cmd.cmdCode == 0x10 ||
                                   cmd.cmdCode == 0x17 || cmd.cmdCode == 0x08 ||
                                   cmd.cmdCode == 0x2B || cmd.cmdCode == 0x14 ||
                                   cmd.cmdCode == 0x15);

                m_addrEdit->setVisible(needsAddr);
                m_qtyEdit->setVisible(needsQty);
                m_writeAddrEdit->setVisible(needsWriteAddr);
                m_writeQtyEdit->setVisible(false);
                m_valueEdit->setVisible(needsValue);

                bool needsDataType = (cmd.cmdCode == 0x03 || cmd.cmdCode == 0x04);
                m_dataTypeCombo->setVisible(needsDataType);
                m_byteOrderCombo->setVisible(needsDataType);

                // Update form labels
                auto* formLayout = qobject_cast<QFormLayout*>(m_paramPanel->layout());
                if (formLayout) {
                    // Show/hide label+widget rows by accessing the label item
                    for (int i = 0; i < formLayout->rowCount(); ++i) {
                        auto* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
                        auto* fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
                        if (labelItem && labelItem->widget()) {
                            bool visible = false;
                            QString text = labelItem->widget()->objectName();
                            if (labelItem->widget() == formLayout->labelForField(m_addrEdit))
                                visible = needsAddr;
                            else if (labelItem->widget() == formLayout->labelForField(m_qtyEdit))
                                visible = needsQty;
                            else if (labelItem->widget() == formLayout->labelForField(m_writeAddrEdit))
                                visible = needsWriteAddr;
                            else if (labelItem->widget() == formLayout->labelForField(m_writeQtyEdit))
                                visible = false;
                            else if (labelItem->widget() == formLayout->labelForField(m_valueEdit))
                                visible = needsValue;
                            else if (labelItem->widget() == formLayout->labelForField(m_dataTypeCombo))
                                visible = (cmd.cmdCode == 0x03 || cmd.cmdCode == 0x04);
                            else if (labelItem->widget() == formLayout->labelForField(m_byteOrderCombo))
                                visible = (cmd.cmdCode == 0x03 || cmd.cmdCode == 0x04);
                            labelItem->widget()->setVisible(visible);
                        }
                    }
                }
                break;
            }
        }
    });
}

void ModbusCommandWidget::populateTable()
{
    auto colors = categoryColors();
    m_table->setRowCount(static_cast<int>(m_commands.size()));

    for (int i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];

        // Type label with color
        auto* typeLabel = new QLabel(cmd.category);
        QString color = colors.value(cmd.category, "#9E9E9E");
        typeLabel->setStyleSheet(QString("color: white; background: %1; "
                                         "border-radius: 3px; padding: 2px 6px; "
                                         "font-size: 11px;").arg(color));
        typeLabel->setAlignment(Qt::AlignCenter);
        m_table->setCellWidget(i, 0, typeLabel);

        // Function code
        auto* codeItem = new QTableWidgetItem(QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper());
        codeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, codeItem);

        // Name
        m_table->setItem(i, 2, new QTableWidgetItem(cmd.name));
    }

    // Select first row
    if (m_table->rowCount() > 0)
        m_table->selectRow(0);
}

void ModbusCommandWidget::applyFilter(const QString& text)
{
    QString category = m_categoryCombo->currentText();
    if (category == "全部类别") category.clear();

    for (int i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];

        bool matchCategory = category.isEmpty() || cmd.category == category;
        bool matchText = text.isEmpty() ||
                         cmd.name.contains(text, Qt::CaseInsensitive) ||
                         QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).contains(text, Qt::CaseInsensitive);

        m_table->setRowHidden(i, !(matchCategory && matchText));
    }
}

void ModbusCommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    auto cmd = ModbusProtocol::commandDefRaw(resp.cmdCode);

    QString html;
    html += QString("<b>[%1] 0x%2</b><br>").arg(cmd.name).arg(resp.cmdCode, 2, 16, QChar('0')).toUpper();

    if (ModbusFrameBuilder::isException(resp.cmdCode) && !resp.rawData.isEmpty()) {
        quint8 excCode = static_cast<quint8>(resp.rawData[0]);
        QString excMsg;
        switch (excCode) {
        case 1: excMsg = "非法功能码"; break;
        case 2: excMsg = "非法数据地址"; break;
        case 3: excMsg = "非法数据值"; break;
        case 4: excMsg = "从站故障"; break;
        case 5: excMsg = "确认"; break;
        case 6: excMsg = "从站忙"; break;
        case 7: excMsg = "负确认"; break;
        case 8: excMsg = "内存奇偶错误"; break;
        default: excMsg = QString("未知(%1)").arg(excCode); break;
        }
        html += QString("<span style='color:red;'>异常: %1 (0x%2)</span><br>")
                    .arg(excMsg).arg(excCode, 2, 16, QChar('0')).toUpper();
    } else if (resp.isValid) {
        html += QString("<span style='color:green;'>成功</span><br>");
    }

    html += QString("<br>%1").arg(interpretation);

    m_resultDisplay->setHtml(html);
}
