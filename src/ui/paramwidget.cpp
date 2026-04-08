#include "paramwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QTreeWidgetItem>

ParamWidget::ParamWidget(QWidget *parent)
    : QWidget(parent)
    , m_paramTree(nullptr)
    , m_paramDetailTable(nullptr)
    , m_valueEdit(nullptr)
    , m_readBtn(nullptr)
    , m_writeBtn(nullptr)
{
    setupUi();
}

void ParamWidget::setupUi()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // Left panel - parameter tree
    QGroupBox *treeGroup = new QGroupBox("参数列表", this);
    QVBoxLayout *treeLayout = new QVBoxLayout(treeGroup);

    m_paramTree = new QTreeWidget(this);
    m_paramTree->setHeaderLabels(QStringList() << "参数号" << "名称");
    m_paramTree->header()->setSectionResizeMode(QHeaderView::Stretch);

    // Categories and their child parameters
    QStringList categories = QStringList() << "坐标系统" << "进给参数" << "主轴参数" << "刀具参数" << "通讯参数";
    QStringList paramsInCategory = QStringList() << "001" << "002" << "003";

    for (const QString &category : categories) {
        QTreeWidgetItem *categoryItem = new QTreeWidgetItem(m_paramTree);
        categoryItem->setText(0, category);
        categoryItem->setText(1, "");
        categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsSelectable);

        for (const QString &paramNo : paramsInCategory) {
            QTreeWidgetItem *paramItem = new QTreeWidgetItem(categoryItem);
            paramItem->setText(0, paramNo);
            paramItem->setText(1, QString("参数%1").arg(paramNo));
        }
    }

    m_paramTree->expandAll();
    treeLayout->addWidget(m_paramTree);

    // Right panel - parameter detail
    QGroupBox *detailGroup = new QGroupBox("参数详情", this);
    QVBoxLayout *detailLayout = new QVBoxLayout(detailGroup);

    m_paramDetailTable = new QTableWidget(4, 2, this);
    m_paramDetailTable->setHorizontalHeaderLabels(QStringList() << "项目" << "值");
    m_paramDetailTable->setItem(0, 0, new QTableWidgetItem("参数号"));
    m_paramDetailTable->setItem(1, 0, new QTableWidgetItem("名称"));
    m_paramDetailTable->setItem(2, 0, new QTableWidgetItem("最小值"));
    m_paramDetailTable->setItem(3, 0, new QTableWidgetItem("最大值"));

    m_paramDetailTable->setItem(0, 1, new QTableWidgetItem("—"));
    m_paramDetailTable->setItem(1, 1, new QTableWidgetItem("—"));
    m_paramDetailTable->setItem(2, 1, new QTableWidgetItem("—"));
    m_paramDetailTable->setItem(3, 1, new QTableWidgetItem("—"));

    m_paramDetailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_paramDetailTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_paramDetailTable->verticalHeader()->setVisible(false);

    detailLayout->addWidget(m_paramDetailTable);

    // Value row
    QHBoxLayout *valueLayout = new QHBoxLayout();
    QLabel *valueLabel = new QLabel("值:", this);
    m_valueEdit = new QLineEdit(this);
    m_valueEdit->setPlaceholderText("输入值...");
    valueLayout->addWidget(valueLabel);
    valueLayout->addWidget(m_valueEdit);

    detailLayout->addLayout(valueLayout);

    // Button row
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_readBtn = new QPushButton("读取", this);
    m_writeBtn = new QPushButton("写入", this);

    m_readBtn->setStyleSheet("QPushButton { background-color: #3498DB; color: white; border: none; padding: 6px 12px; }");
    m_writeBtn->setStyleSheet("QPushButton { background-color: #E67E22; color: white; border: none; padding: 6px 12px; }");

    buttonLayout->addWidget(m_readBtn);
    buttonLayout->addWidget(m_writeBtn);

    detailLayout->addLayout(buttonLayout);

    // Add widgets to splitter
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(treeGroup);
    splitter->addWidget(detailGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
    setLayout(mainLayout);

    connect(m_paramTree, &QTreeWidget::itemClicked, this, &ParamWidget::onParamSelected);
    connect(m_readBtn, &QPushButton::clicked, this, &ParamWidget::onReadClicked);
    connect(m_writeBtn, &QPushButton::clicked, this, &ParamWidget::onWriteClicked);
}

void ParamWidget::onParamSelected(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (item == nullptr) {
        return;
    }

    // Check if item has a parent (i.e., it's a child item, not a category)
    QTreeWidgetItem *parent = item->parent();
    if (parent == nullptr) {
        // This is a category, clear details
        m_paramDetailTable->item(0, 1)->setText("—");
        m_paramDetailTable->item(1, 1)->setText("—");
        m_paramDetailTable->item(2, 1)->setText("—");
        m_paramDetailTable->item(3, 1)->setText("—");
        m_valueEdit->clear();
        return;
    }

    QString paramNo = item->text(0);
    QString name = item->text(1);

    m_paramDetailTable->item(0, 1)->setText(paramNo);
    m_paramDetailTable->item(1, 1)->setText(name);
    m_paramDetailTable->item(2, 1)->setText("0");
    m_paramDetailTable->item(3, 1)->setText("9999");
}

void ParamWidget::onReadClicked()
{
    QString paramNoStr = m_paramDetailTable->item(0, 1)->text();
    if (paramNoStr == "—") {
        return;
    }
    bool ok;
    quint16 paramNo = paramNoStr.toUInt(&ok);
    if (ok) {
        emit readParam(paramNo);
    }
}

void ParamWidget::onWriteClicked()
{
    QString paramNoStr = m_paramDetailTable->item(0, 1)->text();
    if (paramNoStr == "—") {
        return;
    }
    bool ok;
    quint16 paramNo = paramNoStr.toUInt(&ok);
    if (ok) {
        emit writeParam(paramNo, m_valueEdit->text());
    }
}