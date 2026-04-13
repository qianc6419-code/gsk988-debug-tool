#include "siemenscommandwidget.h"
#include "siemensprotocol.h"
#include <QSplitter>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <cstring>

SiemensCommandWidget::SiemensCommandWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    populateCommandTable();
}

void SiemensCommandWidget::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // === Left: command table ===
    auto* leftLayout = new QVBoxLayout;

    auto* filterLayout = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索命令..."));
    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(QStringLiteral("全部"));
    filterLayout->addWidget(m_searchEdit);
    filterLayout->addWidget(m_categoryCombo);
    leftLayout->addLayout(filterLayout);

    m_cmdTable = new QTableWidget;
    m_cmdTable->setColumnCount(4);
    m_cmdTable->setHorizontalHeaderLabels({
        QStringLiteral("功能码"), QStringLiteral("名称"),
        QStringLiteral("类别"), QStringLiteral("数据类型")
    });
    m_cmdTable->horizontalHeader()->setStretchLastSection(true);
    m_cmdTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cmdTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cmdTable->verticalHeader()->setVisible(false);
    leftLayout->addWidget(m_cmdTable);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < m_cmdTable->rowCount(); ++i) {
            bool match = false;
            for (int j = 0; j < m_cmdTable->columnCount(); ++j) {
                if (m_cmdTable->item(i, j)->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            m_cmdTable->setRowHidden(i, !match);
        }
    });

    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        QString cat = m_categoryCombo->itemText(index);
        for (int i = 0; i < m_cmdTable->rowCount(); ++i) {
            if (index == 0) {
                m_cmdTable->setRowHidden(i, false);
            } else {
                m_cmdTable->setRowHidden(i, m_cmdTable->item(i, 2)->text() != cat);
            }
        }
    });

    connect(m_cmdTable, &QTableWidget::currentCellChanged,
            this, &SiemensCommandWidget::onCurrentCellChanged);

    // === Right: params + send + result ===
    auto* rightLayout = new QVBoxLayout;

    // Parameter stack
    m_paramStack = new QStackedWidget;

    // Page 0: No params
    m_noParamPage = new QWidget;
    {
        auto* l = new QVBoxLayout(m_noParamPage);
        l->addWidget(new QLabel(QStringLiteral("此命令无额外参数")));
    }
    m_paramStack->addWidget(m_noParamPage);

    // Page 1: Alarm params
    m_alarmParamPage = new QWidget;
    {
        auto* l = new QGridLayout(m_alarmParamPage);
        m_alarmIndexSpin = new QSpinBox;
        m_alarmIndexSpin->setRange(1, 32);
        m_alarmIndexSpin->setValue(1);
        l->addWidget(new QLabel(QStringLiteral("告警索引:")), 0, 0);
        l->addWidget(m_alarmIndexSpin, 0, 1);
    }
    m_paramStack->addWidget(m_alarmParamPage);

    // Page 2: Read R params
    m_readRParamPage = new QWidget;
    {
        auto* l = new QGridLayout(m_readRParamPage);
        m_readRAddrSpin = new QSpinBox;
        m_readRAddrSpin->setRange(0, 9999);
        m_readRAddrSpin->setValue(1);
        l->addWidget(new QLabel(QStringLiteral("R变量地址:")), 0, 0);
        l->addWidget(m_readRAddrSpin, 0, 1);
    }
    m_paramStack->addWidget(m_readRParamPage);

    // Page 3: Write R params
    m_writeRParamPage = new QWidget;
    {
        auto* l = new QGridLayout(m_writeRParamPage);
        m_writeRAddrSpin = new QSpinBox;
        m_writeRAddrSpin->setRange(0, 9999);
        m_writeRAddrSpin->setValue(1);
        m_writeRValueSpin = new QDoubleSpinBox;
        m_writeRValueSpin->setRange(-999999.0, 999999.0);
        m_writeRValueSpin->setDecimals(4);
        l->addWidget(new QLabel(QStringLiteral("R变量地址:")), 0, 0);
        l->addWidget(m_writeRAddrSpin, 0, 1);
        l->addWidget(new QLabel(QStringLiteral("写入值:")), 1, 0);
        l->addWidget(m_writeRValueSpin, 1, 1);
    }
    m_paramStack->addWidget(m_writeRParamPage);

    // Page 4: R Driver params
    m_rDriverParamPage = new QWidget;
    {
        auto* l = new QGridLayout(m_rDriverParamPage);
        m_rDriverAxisSpin = new QSpinBox;
        m_rDriverAxisSpin->setRange(0, 255);
        m_rDriverAxisSpin->setValue(0xA1);
        m_rDriverAxisSpin->setDisplayIntegerBase(16);
        m_rDriverAddrSpin = new QSpinBox;
        m_rDriverAddrSpin->setRange(0, 255);
        m_rDriverAddrSpin->setValue(0x25);
        m_rDriverAddrSpin->setDisplayIntegerBase(16);
        m_rDriverIndexSpin = new QSpinBox;
        m_rDriverIndexSpin->setRange(0, 255);
        m_rDriverIndexSpin->setValue(0x01);
        m_rDriverIndexSpin->setDisplayIntegerBase(16);
        l->addWidget(new QLabel(QStringLiteral("轴标志(Hex):")), 0, 0);
        l->addWidget(m_rDriverAxisSpin, 0, 1);
        l->addWidget(new QLabel(QStringLiteral("地址(Hex):")), 1, 0);
        l->addWidget(m_rDriverAddrSpin, 1, 1);
        l->addWidget(new QLabel(QStringLiteral("索引(Hex):")), 2, 0);
        l->addWidget(m_rDriverIndexSpin, 2, 1);
    }
    m_paramStack->addWidget(m_rDriverParamPage);

    // Page 5: PLC params
    m_plcParamPage = new QWidget;
    {
        auto* l = new QGridLayout(m_plcParamPage);
        m_plcOffsetSpin = new QSpinBox;
        m_plcOffsetSpin->setRange(0, 30);
        m_plcOffsetSpin->setValue(0);
        l->addWidget(new QLabel(QStringLiteral("DB1600字节偏移:")), 0, 0);
        l->addWidget(m_plcOffsetSpin, 0, 1);
    }
    m_paramStack->addWidget(m_plcParamPage);

    rightLayout->addWidget(m_paramStack);

    // Send button
    m_sendBtn = new QPushButton(QStringLiteral("发送"));
    m_sendBtn->setEnabled(false);
    connect(m_sendBtn, &QPushButton::clicked, this, &SiemensCommandWidget::onSendClicked);
    rightLayout->addWidget(m_sendBtn);

    // Result display
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setMaximumHeight(200);
    rightLayout->addWidget(m_resultDisplay);
    rightLayout->addStretch();

    // Layout
    auto* splitter = new QSplitter(Qt::Horizontal);
    auto* leftWidget = new QWidget;
    leftWidget->setLayout(leftLayout);
    auto* rightWidget = new QWidget;
    rightWidget->setLayout(rightLayout);
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->addWidget(splitter);
}

void SiemensCommandWidget::populateCommandTable()
{
    auto cmds = SiemensProtocol::allCommandsRaw();
    m_cmdTable->setRowCount(cmds.size());

    // Collect unique categories
    QStringList categories;
    for (const auto& c : cmds) {
        if (!categories.contains(c.category))
            categories << c.category;
    }
    for (const auto& cat : categories)
        m_categoryCombo->addItem(cat);

    for (int i = 0; i < cmds.size(); ++i) {
        const auto& c = cmds[i];
        m_cmdTable->setItem(i, 0, new QTableWidgetItem(
            QString("0x%1").arg(c.cmdCode, 2, 16, QChar('0')).toUpper()));
        m_cmdTable->setItem(i, 1, new QTableWidgetItem(c.name));
        m_cmdTable->setItem(i, 2, new QTableWidgetItem(c.category));
        m_cmdTable->setItem(i, 3, new QTableWidgetItem(c.dataType));

        // Store cmdCode in item data
        m_cmdTable->item(i, 0)->setData(Qt::UserRole, c.cmdCode);
    }

    m_cmdTable->resizeColumnsToContents();
}

void SiemensCommandWidget::onCurrentCellChanged(int row, int col, int prevRow, int prevCol)
{
    Q_UNUSED(col);
    Q_UNUSED(prevRow);
    Q_UNUSED(prevCol);

    if (row < 0) {
        m_sendBtn->setEnabled(false);
        return;
    }

    auto* item = m_cmdTable->item(row, 0);
    if (!item) {
        m_sendBtn->setEnabled(false);
        return;
    }

    quint8 cmdCode = static_cast<quint8>(item->data(Qt::UserRole).toInt());
    m_sendBtn->setEnabled(true);

    // Show appropriate param page
    switch (cmdCode) {
    case 0x2E: // Alarm
        m_paramStack->setCurrentWidget(m_alarmParamPage);
        break;
    case 0x2F: // Read R
        m_paramStack->setCurrentWidget(m_readRParamPage);
        break;
    case 0x30: // Write R
        m_paramStack->setCurrentWidget(m_writeRParamPage);
        break;
    case 0x31: // R Driver
        m_paramStack->setCurrentWidget(m_rDriverParamPage);
        break;
    case 0x34: // PLC alarm
        m_paramStack->setCurrentWidget(m_plcParamPage);
        break;
    default:
        m_paramStack->setCurrentWidget(m_noParamPage);
        break;
    }
}

void SiemensCommandWidget::onSendClicked()
{
    int row = m_cmdTable->currentRow();
    if (row < 0) return;

    auto* item = m_cmdTable->item(row, 0);
    if (!item) return;

    quint8 cmdCode = static_cast<quint8>(item->data(Qt::UserRole).toInt());
    QByteArray params;

    switch (cmdCode) {
    case 0x2E: { // Alarm
        params.append(static_cast<char>(m_alarmIndexSpin->value()));
        break;
    }
    case 0x2F: { // Read R
        qint32 addr = m_readRAddrSpin->value();
        params.resize(4);
        std::memcpy(params.data(), &addr, 4);
        break;
    }
    case 0x30: { // Write R
        qint32 addr = m_writeRAddrSpin->value();
        double val = m_writeRValueSpin->value();
        params.resize(12);
        std::memcpy(params.data(), &addr, 4);
        std::memcpy(params.data() + 4, &val, 8);
        break;
    }
    case 0x31: { // R Driver
        params.append(static_cast<char>(m_rDriverAxisSpin->value()));
        params.append(static_cast<char>(m_rDriverAddrSpin->value()));
        params.append(static_cast<char>(m_rDriverIndexSpin->value()));
        break;
    }
    case 0x34: { // PLC alarm
        qint32 offset = m_plcOffsetSpin->value();
        params.append(reinterpret_cast<const char*>(&offset), 4);
        break;
    }
    default:
        break;
    }

    m_resultDisplay->clear();
    emit sendCommand(cmdCode, params);
}

void SiemensCommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    QStringList hex;
    for (int i = 0; i < resp.rawData.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(resp.rawData[i]), 2, 16, QChar('0')).toUpper();

    m_resultDisplay->append(QStringLiteral("=== 响应 ==="));
    m_resultDisplay->append(QStringLiteral("状态: %1").arg(resp.isValid ? "成功" : "失败"));
    m_resultDisplay->append(QStringLiteral("解析: %1").arg(interpretation));
    m_resultDisplay->append(QStringLiteral("HEX: %1").arg(hex.join(" ")));
}
