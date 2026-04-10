#include "commandwidget.h"
#include "protocol/gsk988protocol.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QRegularExpression>

// Category colors
static QMap<QString, QString> categoryColors()
{
    return {
        {"通讯管理", "#4CAF50"},
        {"实时信息", "#2196F3"},
        {"参数",     "#FF9800"},
        {"刀补",     "#9C27B0"},
        {"宏变量",   "#00BCD4"},
        {"丝杠补偿", "#795548"},
        {"轴IO",     "#607D8B"},
        {"报警诊断", "#F44336"},
        {"其他",     "#9E9E9E"},
    };
}

CommandWidget::CommandWidget(QWidget* parent)
    : QWidget(parent)
    , m_commands(Gsk988Protocol::allCommands())
{
    setupUI();
    populateTable();
}

void CommandWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Filter bar
    auto* filterBar = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("搜索命令名/命令码...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandWidget::applyFilter);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem("全部类别");
    QSet<QString> categories;
    for (const auto& c : m_commands) categories.insert(c.category);
    for (const auto& cat : categories) m_categoryCombo->addItem(cat);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { applyFilter(m_searchEdit->text()); });

    filterBar->addWidget(m_searchEdit);
    filterBar->addWidget(m_categoryCombo);
    mainLayout->addLayout(filterBar);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"类型", "命令码", "名称", "通道", "参数", "操作"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    mainLayout->addWidget(m_table, 3);

    // Result display
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setMaximumHeight(150);
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");

    mainLayout->addWidget(m_resultDisplay, 1);
}

void CommandWidget::populateTable()
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

        // Command code
        auto* codeItem = new QTableWidgetItem(QString("0x%1").arg(cmd.cmdCode, 2, 16, QChar('0')).toUpper());
        codeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, codeItem);

        // Name
        m_table->setItem(i, 2, new QTableWidgetItem(cmd.name));

        // Channel input (if needed)
        bool needsChannel = cmd.requestParams.contains("通道号");
        if (needsChannel) {
            auto* spin = new QSpinBox;
            spin->setRange(0, 255);
            spin->setValue(0);
            m_table->setCellWidget(i, 3, spin);
        }

        // Parameter input (if needed, excluding channel)
        bool needsParam = cmd.requestParams.contains("参数号") ||
                          cmd.requestParams.contains("轴类型") ||
                          cmd.requestParams.contains("速度类型") ||
                          cmd.requestParams.contains("诊断号") ||
                          cmd.requestParams.contains("变量号") ||
                          cmd.requestParams.contains("刀补号") ||
                          cmd.requestParams.contains("组号") ||
                          cmd.requestParams.contains("值类型") ||
                          cmd.requestParams.contains("寄存器") ||
                          cmd.requestParams.contains("IO类型") ||
                          cmd.requestParams.contains("索引") ||
                          cmd.requestParams.contains("轴号") ||
                          cmd.requestParams.contains("补偿号");
        if (needsParam) {
            auto* paramEdit = new QLineEdit;
            paramEdit->setPlaceholderText(cmd.requestParams);
            paramEdit->setMaximumWidth(150);
            m_table->setCellWidget(i, 4, paramEdit);
        }

        // Send button
        auto* sendBtn = new QPushButton("发送");
        connect(sendBtn, &QPushButton::clicked, this, [this, i]() {
            const auto& cmd = m_commands[i];
            QByteArray params;

            // Channel
            auto* spin = qobject_cast<QSpinBox*>(m_table->cellWidget(i, 3));
            if (spin) {
                params.append(static_cast<char>(spin->value()));
            }

            // Extra params (hex input)
            auto* paramEdit = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 4));
            if (paramEdit && !paramEdit->text().isEmpty()) {
                QString text = paramEdit->text();
                text.remove(' ');
                text.remove(',');
                text.remove("0x", Qt::CaseInsensitive);
                params.append(QByteArray::fromHex(text.toUtf8()));
            }

            emit sendCommand(cmd.cmdCode, params);
        });
        m_table->setCellWidget(i, 5, sendBtn);
    }
}

void CommandWidget::applyFilter(const QString& text)
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

void CommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    auto cmd = Gsk988Protocol::commandDef(resp.cmdCode);

    QString html;
    html += QString("<b>[%1] %2</b><br>").arg(cmd.name).arg(resp.cmdCode, 2, 16, QChar('0')).toUpper();

    if (resp.isValid) {
        html += QString("<span style='color:green;'>成功</span> (错误码: 0x%1)<br>")
                    .arg(resp.errorCode, 4, 16, QChar('0'));
    } else {
        html += QString("<span style='color:red;'>%1</span><br>").arg(resp.errorString);
    }

    html += QString("<br>%1").arg(interpretation);

    m_resultDisplay->setHtml(html);
}
