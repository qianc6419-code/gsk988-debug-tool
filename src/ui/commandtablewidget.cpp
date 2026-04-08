#include "commandtablewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QSpinBox>
#include <QMessageBox>

CommandTableWidget::CommandTableWidget(QWidget *parent)
    : QWidget(parent)
    , m_searchEdit(new QLineEdit(this))
    , m_categoryCombo(new QComboBox(this))
    , m_functionCombo(new QComboBox(this))
    , m_table(new QTableWidget(this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Filter bar
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel(tr("搜索:")), 0);
    filterLayout->addWidget(m_searchEdit, 1);

    m_categoryCombo->addItem(tr("全部"), QString());
    m_categoryCombo->addItem(tr("读"), QString("读"));
    m_categoryCombo->addItem(tr("写"), QString("写"));
    m_categoryCombo->addItem(tr("控制"), QString("控制"));
    filterLayout->addWidget(new QLabel(tr("类型:")), 0);
    filterLayout->addWidget(m_categoryCombo, 0);

    m_functionCombo->addItem(tr("全部"), QString());
    m_functionCombo->addItem(tr("设备信息"), QString("设备信息"));
    m_functionCombo->addItem(tr("状态坐标"), QString("状态坐标"));
    m_functionCombo->addItem(tr("报警诊断"), QString("报警诊断"));
    m_functionCombo->addItem(tr("参数操作"), QString("参数操作"));
    m_functionCombo->addItem(tr("运行控制"), QString("运行控制"));
    m_functionCombo->addItem(tr("PLC"), QString("PLC"));
    m_functionCombo->addItem(tr("数据块"), QString("数据块"));
    filterLayout->addWidget(new QLabel(tr("功能:")), 0);
    filterLayout->addWidget(m_functionCombo, 0);

    mainLayout->addLayout(filterLayout);

    // Table
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(QStringList() << tr("类型") << tr("命令码") << tr("名称") << tr("说明") << tr("快捷参数") << tr("操作"));
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchToContentsMode(QHeaderView::Stretch);

    mainLayout->addWidget(m_table);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandTableWidget::onSearchChanged);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CommandTableWidget::onCategoryChanged);
    connect(m_functionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CommandTableWidget::onFunctionChanged);
}

void CommandTableWidget::populateCommands()
{
    m_allCommands = CommandRegistry::instance()->allCommands();
    filterCommands();
}

void CommandTableWidget::filterCommands()
{
    m_filteredCommands.clear();

    QString searchText = m_searchEdit->text().toLower();
    QString category = m_categoryCombo->currentData().toString();
    QString function = m_functionCombo->currentData().toString();

    for (const CommandInfo &cmd : m_allCommands) {
        // Category filter
        if (!category.isEmpty() && cmd.category != category) {
            continue;
        }

        // Function filter
        if (!function.isEmpty() && cmd.function != function) {
            continue;
        }

        // Search filter
        if (!searchText.isEmpty()) {
            bool match = false;
            if (cmd.name.toLower().contains(searchText)) {
                match = true;
            } else if (cmd.description.toLower().contains(searchText)) {
                match = true;
            } else if (cmd.name.toLower().contains(searchText)) {
                match = true;
            } else {
                QString cmdHex = QString("0x%1").arg(cmd.cmd, 2, 16, QChar('0')).toUpper();
                if (cmdHex.toLower().contains(searchText)) {
                    match = true;
                }
            }
            if (!match) {
                continue;
            }
        }

        m_filteredCommands.append(cmd);
    }

    // Update table
    m_table->setRowCount(m_filteredCommands.size());

    for (int row = 0; row < m_filteredCommands.size(); ++row) {
        const CommandInfo &cmd = m_filteredCommands[row];

        // Color based on type
        QColor bgColor;
        if (cmd.category == tr("读")) {
            bgColor = QColor("#e8f5e9");  // light green
        } else if (cmd.category == tr("写")) {
            bgColor = QColor("#fff3e0");  // light orange
        } else if (cmd.category == tr("控制")) {
            bgColor = QColor("#ffebee");  // light red
        } else {
            bgColor = Qt::white;
        }

        // Type
        QTableWidgetItem *typeItem = new QTableWidgetItem(cmd.category);
        typeItem->setBackground(QBrush(bgColor));
        m_table->setItem(row, 0, typeItem);

        // Command code
        QTableWidgetItem *cmdItem = new QTableWidgetItem(QString("0x%1").arg(cmd.cmd, 2, 16, QChar('0')).toUpper());
        cmdItem->setBackground(QBrush(bgColor));
        m_table->setItem(row, 1, cmdItem);

        // Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(cmd.name);
        nameItem->setBackground(QBrush(bgColor));
        m_table->setItem(row, 2, nameItem);

        // Description
        QTableWidgetItem *descItem = new QTableWidgetItem(cmd.description);
        descItem->setBackground(QBrush(bgColor));
        m_table->setItem(row, 3, descItem);

        // Shortcut parameter column
        if (cmd.cmd == 0x06 || cmd.cmd == 0x11) {
            QSpinBox *spinBox = new QSpinBox();
            spinBox->setRange(0, 9999);
            spinBox->setProperty("row", row);
            m_table->setCellWidget(row, 4, spinBox);
        } else if (cmd.cmd == 0x20) {
            QComboBox *comboBox = new QComboBox();
            comboBox->addItem(tr("进给保持"), 0);
            comboBox->addItem(tr("复位"), 1);
            comboBox->addItem(tr("急停"), 2);
            comboBox->setProperty("row", row);
            m_table->setCellWidget(row, 4, comboBox);
        } else {
            QTableWidgetItem *paramItem = new QTableWidgetItem(tr("—"));
            paramItem->setBackground(QBrush(bgColor));
            m_table->setItem(row, 4, paramItem);
        }

        // Send button
        QPushButton *sendBtn = new QPushButton(tr("发送"));
        sendBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; padding: 5px 10px; }");
        sendBtn->setProperty("row", row);
        connect(sendBtn, &QPushButton::clicked, this, [this]() {
            QPushButton *btn = qobject_cast<QPushButton*>(sender());
            if (btn) {
                onSendClicked(btn->property("row").toInt());
            }
        });
        m_table->setCellWidget(row, 5, sendBtn);
    }
}

void CommandTableWidget::onSearchChanged(const QString &text)
{
    Q_UNUSED(text);
    filterCommands();
}

void CommandTableWidget::onCategoryChanged(int index)
{
    Q_UNUSED(index);
    filterCommands();
}

void CommandTableWidget::onFunctionChanged(int index)
{
    Q_UNUSED(index);
    filterCommands();
}

void CommandTableWidget::onSendClicked(int row)
{
    if (row < 0 || row >= m_filteredCommands.size()) {
        return;
    }

    const CommandInfo &cmd = m_filteredCommands[row];
    QString param = getParamFromRow(row, cmd);

    QByteArray paramData;
    if (!param.isEmpty() && param != tr("—")) {
        bool ok;
        int paramVal = param.toInt(&ok);
        if (ok) {
            quint16 paramNum = static_cast<quint16>(paramVal);
            paramData.append(reinterpret_cast<char*>(&paramNum), 2);
            paramData.append(param);
        }
    }

    emit commandSendRequested(cmd.cmd, cmd.sub, cmd.type, paramData);
}

QString CommandTableWidget::getParamFromRow(int row, const CommandInfo &cmd)
{
    QWidget *widget = m_table->cellWidget(row, 4);
    if (!widget) {
        return QString();
    }

    if (cmd.cmd == 0x06 || cmd.cmd == 0x11) {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget);
        if (spinBox) {
            return QString::number(spinBox->value());
        }
    } else if (cmd.cmd == 0x20) {
        QComboBox *comboBox = qobject_cast<QComboBox*>(widget);
        if (comboBox) {
            return QString::number(comboBox->currentData().toInt());
        }
    }

    return QString();
}
