#include "fanuccommandwidget.h"
#include "fanucframebuilder.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTextEdit>

// Category colors
static QMap<QString, QString> categoryColors()
{
    return {
        {QStringLiteral("系统信息"), "#4CAF50"},
        {QStringLiteral("运行状态"), "#2196F3"},
        {QStringLiteral("主轴"),     "#FF9800"},
        {QStringLiteral("进给"),     "#9C27B0"},
        {QStringLiteral("计数"),     "#00BCD4"},
        {QStringLiteral("时间"),     "#607D8B"},
        {QStringLiteral("程序"),     "#795548"},
        {QStringLiteral("告警"),     "#F44336"},
        {QStringLiteral("坐标"),     "#009688"},
        {QStringLiteral("宏变量"),   "#E91E63"},
        {QStringLiteral("PMC"),      "#3F51B5"},
        {QStringLiteral("参数"),     "#8BC34A"},
    };
}

FanucCommandWidget::FanucCommandWidget(QWidget* parent)
    : QWidget(parent)
    , m_commands(FanucProtocol::allCommandsRaw())
{
    setupUI();
    populateTable();
}

void FanucCommandWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // ===== Left: command table =====
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Filter bar
    auto* filterBar = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索功能码/名称..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FanucCommandWidget::applyFilter);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(QStringLiteral("全部类别"));
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
    m_table->setHorizontalHeaderLabels({QStringLiteral("类型"),
                                        QStringLiteral("功能码"),
                                        QStringLiteral("名称")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_table, &QTableWidget::currentCellChanged, this, [this](int row) {
        if (row < 0 || row >= m_commands.size()) return;
        // Highlight selected row
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

    // Parameter form panel
    auto* paramPanel = new QWidget;
    auto* paramLayout = new QFormLayout(paramPanel);
    paramLayout->setContentsMargins(0, 0, 0, 0);

    m_addrEdit = new QLineEdit;
    m_addrEdit->setPlaceholderText(QStringLiteral("地址, 如 100 或 0x64"));

    m_valueEdit = new QLineEdit;
    m_valueEdit->setPlaceholderText(QStringLiteral("写入值"));

    m_addrTypeCombo = new QComboBox;
    m_addrTypeCombo->addItem("G");
    m_addrTypeCombo->addItem("F");
    m_addrTypeCombo->addItem("Y");
    m_addrTypeCombo->addItem("X");
    m_addrTypeCombo->addItem("A");
    m_addrTypeCombo->addItem("R");
    m_addrTypeCombo->addItem("T");
    m_addrTypeCombo->addItem("K");
    m_addrTypeCombo->addItem("C");
    m_addrTypeCombo->addItem("D");
    m_addrTypeCombo->addItem("E");

    m_dataTypeCombo = new QComboBox;
    m_dataTypeCombo->addItem(QStringLiteral("byte"), 0);
    m_dataTypeCombo->addItem(QStringLiteral("word"), 1);
    m_dataTypeCombo->addItem(QStringLiteral("long"), 2);

    paramLayout->addRow(QStringLiteral("地址类型:"), m_addrTypeCombo);
    paramLayout->addRow(QStringLiteral("地址:"), m_addrEdit);
    paramLayout->addRow(QStringLiteral("数据类型:"), m_dataTypeCombo);
    paramLayout->addRow(QStringLiteral("写入值:"), m_valueEdit);

    rightLayout->addWidget(paramPanel);

    // Send button
    auto* sendBtn = new QPushButton(QStringLiteral("发送"));
    sendBtn->setFixedHeight(32);
    connect(sendBtn, &QPushButton::clicked, this, [this]() {
        int row = m_table->currentRow();
        if (row < 0) return;

        // Find actual command index (table may be filtered)
        auto* codeItem = m_table->item(row, 1);
        if (!codeItem) return;
        QString codeStr = codeItem->text();

        int cmdIdx = -1;
        for (int j = 0; j < m_commands.size(); ++j) {
            if (QString("0x%1").arg(m_commands[j].cmdCode, 2, 16, QChar('0')).toUpper() == codeStr) {
                cmdIdx = j;
                break;
            }
        }
        if (cmdIdx < 0) return;

        const auto& cmd = m_commands[cmdIdx];
        quint8 funcCode = cmd.cmdCode;

        // Address/value parsing helpers
        auto parseInt = [](const QString& text) -> int {
            QString t = text.trimmed();
            bool ok;
            if (t.startsWith("0x", Qt::CaseInsensitive))
                return t.toInt(&ok, 16);
            return t.toInt(&ok, 10);
        };

        using FB = FanucFrameBuilder;
        QByteArray params;

        switch (funcCode) {
        case 0x16: { // 读宏变量
            int addr = parseInt(m_addrEdit->text());
            params = FB::encodeInt32(addr);
            break;
        }
        case 0x17: { // 写宏变量: addr(4B) + numerator(4B) + precision(2B)
            int addr = parseInt(m_addrEdit->text());
            int value = parseInt(m_valueEdit->text());
            params = FB::encodeInt32(addr);
            params.append(FB::encodeInt32(value));
            params.append(FB::encodeInt16(0)); // precision = 0
            break;
        }
        case 0x18: { // 读PMC: addrType(1B) + startAddr(4B) + endAddr(4B) + dataType(4B)
            char addrType = m_addrTypeCombo->currentText().at(0).toLatin1();
            int addr = parseInt(m_addrEdit->text());
            int dataType = m_dataTypeCombo->currentData().toInt();
            // Calculate endAddr based on data type size: byte=0, word=1, long=3
            int sizeBytes[] = {1, 2, 4};
            int endAddr = addr + sizeBytes[dataType] - 1;
            params.append(addrType);
            params.append(FB::encodeInt32(addr));
            params.append(FB::encodeInt32(endAddr));
            params.append(FB::encodeInt32(dataType));
            break;
        }
        case 0x19: { // 写PMC: addrType(1B) + startAddr(4B) + endAddr(4B) + dataType(4B) + value
            char addrType = m_addrTypeCombo->currentText().at(0).toLatin1();
            int addr = parseInt(m_addrEdit->text());
            int dataType = m_dataTypeCombo->currentData().toInt();
            int sizeBytes[] = {1, 2, 4};
            int endAddr = addr + sizeBytes[dataType] - 1;
            params.append(addrType);
            params.append(FB::encodeInt32(addr));
            params.append(FB::encodeInt32(endAddr));
            params.append(FB::encodeInt32(dataType));
            // Append value based on data type
            int value = parseInt(m_valueEdit->text());
            switch (dataType) {
            case 0: // byte
                params.append(static_cast<char>(static_cast<quint8>(value)));
                break;
            case 1: // word
                params.append(FB::encodeInt16(static_cast<qint16>(value)));
                break;
            case 2: // long
                params.append(FB::encodeInt32(static_cast<qint32>(value)));
                break;
            default:
                break;
            }
            break;
        }
        case 0x1A: { // 读参数
            int paramNo = parseInt(m_addrEdit->text());
            params = FB::encodeInt32(paramNo);
            break;
        }
        default:
            // Commands 0x01-0x15: no params needed
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
        auto* codeItem = m_table->item(row, 1);
        if (!codeItem) return;
        QString codeStr = codeItem->text();
        for (const auto& cmd : m_commands) {
            if (QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper() == codeStr) {
                updateParamVisibility(cmd.cmdCode);
                break;
            }
        }
    });
}

void FanucCommandWidget::populateTable()
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
        auto* codeItem = new QTableWidgetItem(
            QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper());
        codeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, codeItem);

        // Name
        m_table->setItem(i, 2, new QTableWidgetItem(cmd.name));
    }

    // Select first row
    if (m_table->rowCount() > 0)
        m_table->selectRow(0);
}

void FanucCommandWidget::applyFilter(const QString& text)
{
    QString category = m_categoryCombo->currentText();
    if (category == QStringLiteral("全部类别")) category.clear();

    for (int i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];

        bool matchCategory = category.isEmpty() || cmd.category == category;
        bool matchText = text.isEmpty() ||
                         cmd.name.contains(text, Qt::CaseInsensitive) ||
                         QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).contains(text, Qt::CaseInsensitive);

        m_table->setRowHidden(i, !(matchCategory && matchText));
    }
}

void FanucCommandWidget::updateParamVisibility(quint8 cmdCode)
{
    bool needsAddr = false;
    bool needsValue = false;
    bool needsAddrType = false;
    bool needsDataType = false;

    switch (cmdCode) {
    case 0x16: // 读宏变量
        needsAddr = true;
        break;
    case 0x17: // 写宏变量
        needsAddr = true;
        needsValue = true;
        break;
    case 0x18: // 读PMC
        needsAddrType = true;
        needsAddr = true;
        needsDataType = true;
        break;
    case 0x19: // 写PMC
        needsAddrType = true;
        needsAddr = true;
        needsValue = true;
        needsDataType = true;
        break;
    case 0x1A: // 读参数
        needsAddr = true;
        break;
    default:
        // 0x01-0x15: no params
        break;
    }

    m_addrEdit->setVisible(needsAddr);
    m_valueEdit->setVisible(needsValue);
    m_addrTypeCombo->setVisible(needsAddrType);
    m_dataTypeCombo->setVisible(needsDataType);

    // Show/hide the form row labels as well
    auto* formLayout = qobject_cast<QFormLayout*>(
        m_addrEdit->parentWidget() ? m_addrEdit->parentWidget()->layout() : nullptr);
    if (formLayout) {
        for (int i = 0; i < formLayout->rowCount(); ++i) {
            auto* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
            if (labelItem && labelItem->widget()) {
                QWidget* label = labelItem->widget();
                if (label == formLayout->labelForField(m_addrEdit))
                    label->setVisible(needsAddr);
                else if (label == formLayout->labelForField(m_valueEdit))
                    label->setVisible(needsValue);
                else if (label == formLayout->labelForField(m_addrTypeCombo))
                    label->setVisible(needsAddrType);
                else if (label == formLayout->labelForField(m_dataTypeCombo))
                    label->setVisible(needsDataType);
            }
        }
    }
}

void FanucCommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    auto cmd = FanucProtocol::commandDefRaw(resp.cmdCode);

    QString html;
    html += QString("<b>[%1] 0x%2</b><br>")
                .arg(cmd.name)
                .arg(resp.cmdCode, 2, 16, QChar('0')).toUpper();

    if (resp.isValid) {
        html += QString("<span style='color:green;'>成功</span><br>");
    } else {
        html += QString("<span style='color:red;'>无效响应</span><br>");
    }

    html += QString("<br>%1").arg(interpretation);

    m_resultDisplay->setHtml(html);
}
