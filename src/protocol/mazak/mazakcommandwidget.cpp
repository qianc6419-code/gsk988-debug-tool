#include "protocol/mazak/mazakcommandwidget.h"
#include "protocol/mazak/mazakdllwrapper.h"
#include "protocol/mazak/mazakframebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>

MazakCommandWidget::MazakCommandWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MazakCommandWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    m_tabs = new QTabWidget;
    mainLayout->addWidget(m_tabs);

    // Tab 1: NC 参数
    m_tabs->addTab(createParamTab("NC参数", 0), "NC参数");

    // Tab 2: 宏变量
    auto* cmnVarWidget = new QWidget;
    auto* cmnLayout = new QVBoxLayout(cmnVarWidget);
    auto* cmnCtrlLayout = new QHBoxLayout;
    cmnCtrlLayout->addWidget(new QLabel("类型:"));
    m_cmnVarTypeCombo = new QComboBox;
    m_cmnVarTypeCombo->addItems({"0", "1", "2", "3"});
    cmnCtrlLayout->addWidget(m_cmnVarTypeCombo);
    cmnCtrlLayout->addWidget(new QLabel("起始:"));
    m_cmnVarStartSpin = new QSpinBox; m_cmnVarStartSpin->setRange(0, 9999); m_cmnVarStartSpin->setValue(100);
    cmnCtrlLayout->addWidget(m_cmnVarStartSpin);
    cmnCtrlLayout->addWidget(new QLabel("数量:"));
    m_cmnVarCountSpin = new QSpinBox; m_cmnVarCountSpin->setRange(1, 100); m_cmnVarCountSpin->setValue(10);
    cmnCtrlLayout->addWidget(m_cmnVarCountSpin);
    auto* cmnReadBtn = new QPushButton("读取");
    auto* cmnWriteBtn = new QPushButton("写入");
    cmnCtrlLayout->addWidget(cmnReadBtn);
    cmnCtrlLayout->addWidget(cmnWriteBtn);
    cmnLayout->addLayout(cmnCtrlLayout);
    m_cmnVarTable = new QTableWidget(0, 2);
    m_cmnVarTable->setHorizontalHeaderLabels({"编号", "值"});
    m_cmnVarTable->horizontalHeader()->setStretchLastSection(true);
    m_cmnVarTable->setAlternatingRowColors(true);
    cmnLayout->addWidget(m_cmnVarTable);
    m_cmnVarStatusLabel = new QLabel;
    cmnLayout->addWidget(m_cmnVarStatusLabel);
    m_tabs->addTab(cmnVarWidget, "宏变量");

    connect(cmnReadBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_cmnVarStatusLabel->setText("未连接"); return; }
        short type = static_cast<short>(m_cmnVarTypeCombo->currentIndex());
        short start = static_cast<short>(m_cmnVarStartSpin->value());
        short count = static_cast<short>(m_cmnVarCountSpin->value());
        QVector<double> buf(count);
        if (dll.getRangedCmnVar(type, start, count, buf.data())) {
            m_cmnVarTable->setRowCount(count);
            for (short i = 0; i < count; ++i) {
                m_cmnVarTable->setItem(i, 0, new QTableWidgetItem(QString::number(start + i)));
                auto* valItem = new QTableWidgetItem(QString::number(buf[i], 'f', 4));
                m_cmnVarTable->setItem(i, 1, valItem);
            }
            m_cmnVarStatusLabel->setText("读取成功");
        } else {
            m_cmnVarStatusLabel->setText("读取失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });
    connect(cmnWriteBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_cmnVarStatusLabel->setText("未连接"); return; }
        short type = static_cast<short>(m_cmnVarTypeCombo->currentIndex());
        short start = static_cast<short>(m_cmnVarStartSpin->value());
        short count = static_cast<short>(m_cmnVarCountSpin->value());
        QVector<double> buf(count);
        for (short i = 0; i < count; ++i) {
            bool ok;
            buf[i] = m_cmnVarTable->item(i, 1) ? m_cmnVarTable->item(i, 1)->text().toDouble(&ok) : 0.0;
        }
        if (dll.setRangedCmnVar(type, start, count, buf.data())) {
            m_cmnVarStatusLabel->setText("写入成功");
        } else {
            m_cmnVarStatusLabel->setText("写入失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });

    // Tab 3: R 寄存器
    auto* rRegWidget = new QWidget;
    auto* rRegLayout = new QVBoxLayout(rRegWidget);
    auto* rRegCtrlLayout = new QHBoxLayout;
    rRegCtrlLayout->addWidget(new QLabel("起始:"));
    m_rRegStartSpin = new QSpinBox; m_rRegStartSpin->setRange(0, 16383); m_rRegStartSpin->setValue(0);
    rRegCtrlLayout->addWidget(m_rRegStartSpin);
    rRegCtrlLayout->addWidget(new QLabel("数量:"));
    m_rRegCountSpin = new QSpinBox; m_rRegCountSpin->setRange(1, 100); m_rRegCountSpin->setValue(10);
    rRegCtrlLayout->addWidget(m_rRegCountSpin);
    auto* rRegReadBtn = new QPushButton("读取");
    auto* rRegWriteBtn = new QPushButton("写入");
    rRegCtrlLayout->addWidget(rRegReadBtn);
    rRegCtrlLayout->addWidget(rRegWriteBtn);
    rRegLayout->addLayout(rRegCtrlLayout);
    m_rRegTable = new QTableWidget(0, 2);
    m_rRegTable->setHorizontalHeaderLabels({"编号", "值"});
    m_rRegTable->horizontalHeader()->setStretchLastSection(true);
    m_rRegTable->setAlternatingRowColors(true);
    rRegLayout->addWidget(m_rRegTable);
    m_rRegStatusLabel = new QLabel;
    rRegLayout->addWidget(m_rRegStatusLabel);
    m_tabs->addTab(rRegWidget, "R寄存器");

    connect(rRegReadBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_rRegStatusLabel->setText("未连接"); return; }
        int start = m_rRegStartSpin->value();
        int count = m_rRegCountSpin->value();
        QVector<int> buf(count);
        if (dll.getRangedRReg(start, count, buf.data())) {
            m_rRegTable->setRowCount(count);
            for (int i = 0; i < count; ++i) {
                m_rRegTable->setItem(i, 0, new QTableWidgetItem(QString::number(start + i)));
                m_rRegTable->setItem(i, 1, new QTableWidgetItem(QString::number(buf[i])));
            }
            m_rRegStatusLabel->setText("读取成功");
        } else {
            m_rRegStatusLabel->setText("读取失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });
    connect(rRegWriteBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_rRegStatusLabel->setText("未连接"); return; }
        int start = m_rRegStartSpin->value();
        int count = m_rRegCountSpin->value();
        QVector<int> buf(count);
        for (int i = 0; i < count; ++i) {
            bool ok;
            buf[i] = m_rRegTable->item(i, 1) ? m_rRegTable->item(i, 1)->text().toInt(&ok) : 0;
        }
        if (dll.setRangedRReg(start, count, buf.data())) {
            m_rRegStatusLabel->setText("写入成功");
        } else {
            m_rRegStatusLabel->setText("写入失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });

    // Tab 4: 工件坐标系
    m_tabs->addTab(createCoordTab(), "工件坐标系");

    // Tab 5: 刀补
    m_tabs->addTab(createToolTab(), "刀补");

    // Tab 6: 操作
    m_tabs->addTab(createOperationTab(), "操作");
}

QWidget* MazakCommandWidget::createParamTab(const QString& name, int type)
{
    Q_UNUSED(name);
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("起始:"));
    m_paramStartSpin = new QSpinBox; m_paramStartSpin->setRange(0, 99999); m_paramStartSpin->setValue(0);
    ctrlLayout->addWidget(m_paramStartSpin);
    ctrlLayout->addWidget(new QLabel("数量:"));
    m_paramCountSpin = new QSpinBox; m_paramCountSpin->setRange(1, 100); m_paramCountSpin->setValue(10);
    ctrlLayout->addWidget(m_paramCountSpin);
    auto* readBtn = new QPushButton("读取");
    auto* writeBtn = new QPushButton("写入");
    ctrlLayout->addWidget(readBtn);
    ctrlLayout->addWidget(writeBtn);
    layout->addLayout(ctrlLayout);

    m_paramTable = new QTableWidget(0, 2);
    m_paramTable->setHorizontalHeaderLabels({"编号", "值"});
    m_paramTable->horizontalHeader()->setStretchLastSection(true);
    m_paramTable->setAlternatingRowColors(true);
    layout->addWidget(m_paramTable);

    m_paramStatusLabel = new QLabel;
    layout->addWidget(m_paramStatusLabel);

    m_currentParamType = type;

    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_paramStatusLabel->setText("未连接"); return; }
        int start = m_paramStartSpin->value();
        int count = m_paramCountSpin->value();
        QVector<int> buf(count);
        if (dll.getRangedParam(start, count, buf.data())) {
            m_paramTable->setRowCount(count);
            for (int i = 0; i < count; ++i) {
                m_paramTable->setItem(i, 0, new QTableWidgetItem(QString::number(start + i)));
                m_paramTable->setItem(i, 1, new QTableWidgetItem(QString::number(buf[i])));
            }
            m_paramStatusLabel->setText("读取成功");
        } else {
            m_paramStatusLabel->setText("读取失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });
    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_paramStatusLabel->setText("未连接"); return; }
        int start = m_paramStartSpin->value();
        int count = m_paramCountSpin->value();
        QVector<int> buf(count);
        for (int i = 0; i < count; ++i) {
            bool ok;
            buf[i] = m_paramTable->item(i, 1) ? m_paramTable->item(i, 1)->text().toInt(&ok) : 0;
        }
        if (dll.setRangedParam(start, count, buf.data())) {
            m_paramStatusLabel->setText("写入成功");
        } else {
            m_paramStatusLabel->setText("写入失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });

    return widget;
}

QWidget* MazakCommandWidget::createToolTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("起始刀号:"));
    m_toolOffStartSpin = new QSpinBox; m_toolOffStartSpin->setRange(0, 99); m_toolOffStartSpin->setValue(1);
    ctrlLayout->addWidget(m_toolOffStartSpin);
    ctrlLayout->addWidget(new QLabel("数量:"));
    m_toolOffCountSpin = new QSpinBox; m_toolOffCountSpin->setRange(1, 20); m_toolOffCountSpin->setValue(5);
    ctrlLayout->addWidget(m_toolOffCountSpin);
    auto* readBtn = new QPushButton("读取");
    auto* writeBtn = new QPushButton("写入");
    ctrlLayout->addWidget(readBtn);
    ctrlLayout->addWidget(writeBtn);
    layout->addLayout(ctrlLayout);

    m_toolOffTable = new QTableWidget(0, 10);
    m_toolOffTable->setHorizontalHeaderLabels({"刀号", "offset1", "offset2", "offset3",
        "offset4", "offset5", "offset6", "offset7", "offset8", "offset9"});
    m_toolOffTable->horizontalHeader()->setStretchLastSection(true);
    m_toolOffTable->setAlternatingRowColors(true);
    layout->addWidget(m_toolOffTable);

    m_toolOffStatusLabel = new QLabel;
    layout->addWidget(m_toolOffStatusLabel);

    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_toolOffStatusLabel->setText("未连接"); return; }
        int start = m_toolOffStartSpin->value();
        int count = m_toolOffCountSpin->value();
        QVector<MAZ_TOFFCOMP> buf(count);
        if (dll.getRangedToolOffsetComp(start, count, buf.data())) {
            m_toolOffTable->setRowCount(count);
            for (int i = 0; i < count; ++i) {
                m_toolOffTable->setItem(i, 0, new QTableWidgetItem(QString::number(start + i)));
                m_toolOffTable->setItem(i, 1, new QTableWidgetItem(QString::number(buf[i].offset1)));
                m_toolOffTable->setItem(i, 2, new QTableWidgetItem(QString::number(buf[i].offset2)));
                m_toolOffTable->setItem(i, 3, new QTableWidgetItem(QString::number(buf[i].offset3)));
                m_toolOffTable->setItem(i, 4, new QTableWidgetItem(QString::number(buf[i].offset4)));
                m_toolOffTable->setItem(i, 5, new QTableWidgetItem(QString::number(buf[i].offset5)));
                m_toolOffTable->setItem(i, 6, new QTableWidgetItem(QString::number(buf[i].offset6)));
                m_toolOffTable->setItem(i, 7, new QTableWidgetItem(QString::number(buf[i].offset7)));
                m_toolOffTable->setItem(i, 8, new QTableWidgetItem(QString::number(buf[i].offset8)));
                m_toolOffTable->setItem(i, 9, new QTableWidgetItem(QString::number(buf[i].offset9)));
            }
            m_toolOffStatusLabel->setText("读取成功");
        } else {
            m_toolOffStatusLabel->setText("读取失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });
    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_toolOffStatusLabel->setText("未连接"); return; }
        int start = m_toolOffStartSpin->value();
        int count = m_toolOffCountSpin->value();
        QVector<MAZ_TOFFCOMP> buf(count);
        for (int i = 0; i < count; ++i) {
            auto getVal = [&](int col) -> int {
                return m_toolOffTable->item(i, col) ? m_toolOffTable->item(i, col)->text().toInt() : 0;
            };
            buf[i].type = 0;
            buf[i].offset1 = getVal(1); buf[i].offset2 = getVal(2);
            buf[i].offset3 = getVal(3); buf[i].offset4 = getVal(4);
            buf[i].offset5 = getVal(5); buf[i].offset6 = getVal(6);
            buf[i].offset7 = getVal(7); buf[i].offset8 = getVal(8);
            buf[i].offset9 = getVal(9);
        }
        if (dll.setRangedToolOffsetComp(start, count, buf.data())) {
            m_toolOffStatusLabel->setText("写入成功");
        } else {
            m_toolOffStatusLabel->setText("写入失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });

    return widget;
}

QWidget* MazakCommandWidget::createCoordTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("起始坐标系:"));
    m_coordStartSpin = new QSpinBox; m_coordStartSpin->setRange(0, 99); m_coordStartSpin->setValue(1);
    ctrlLayout->addWidget(m_coordStartSpin);
    ctrlLayout->addWidget(new QLabel("数量:"));
    m_coordCountSpin = new QSpinBox; m_coordCountSpin->setRange(1, 20); m_coordCountSpin->setValue(6);
    ctrlLayout->addWidget(m_coordCountSpin);
    auto* readBtn = new QPushButton("读取");
    auto* writeBtn = new QPushButton("写入");
    ctrlLayout->addWidget(readBtn);
    ctrlLayout->addWidget(writeBtn);
    layout->addLayout(ctrlLayout);

    m_coordTable = new QTableWidget(0, 2);
    m_coordTable->setHorizontalHeaderLabels({"编号", "值(双精度)"});
    m_coordTable->horizontalHeader()->setStretchLastSection(true);
    m_coordTable->setAlternatingRowColors(true);
    layout->addWidget(m_coordTable);

    m_coordStatusLabel = new QLabel;
    layout->addWidget(m_coordStatusLabel);

    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_coordStatusLabel->setText("未连接"); return; }
        int start = m_coordStartSpin->value();
        int count = m_coordCountSpin->value();
        QVector<double> buf(count);
        if (dll.getRangedWorkCoordSys(start, count, buf.data())) {
            m_coordTable->setRowCount(count);
            for (int i = 0; i < count; ++i) {
                m_coordTable->setItem(i, 0, new QTableWidgetItem(QString::number(start + i)));
                m_coordTable->setItem(i, 1, new QTableWidgetItem(QString::number(buf[i], 'f', 4)));
            }
            m_coordStatusLabel->setText("读取成功");
        } else {
            m_coordStatusLabel->setText("读取失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });
    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { m_coordStatusLabel->setText("未连接"); return; }
        int start = m_coordStartSpin->value();
        int count = m_coordCountSpin->value();
        QVector<double> buf(count);
        for (int i = 0; i < count; ++i) {
            buf[i] = m_coordTable->item(i, 1) ? m_coordTable->item(i, 1)->text().toDouble() : 0.0;
        }
        if (dll.setRangedWorkCoordSys(start, count, buf.data())) {
            m_coordStatusLabel->setText("写入成功");
        } else {
            m_coordStatusLabel->setText("写入失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });

    return widget;
}

QWidget* MazakCommandWidget::createOperationTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* btnLayout = new QHBoxLayout;
    auto* resetBtn = new QPushButton("NC 复位");
    resetBtn->setMinimumHeight(40);
    btnLayout->addWidget(resetBtn);
    layout->addLayout(btnLayout);

    auto* statusLabel = new QLabel;
    layout->addWidget(statusLabel);
    layout->addStretch();

    connect(resetBtn, &QPushButton::clicked, this, [statusLabel]() {
        auto& dll = MazakDLLWrapper::instance();
        if (!dll.isConnected()) { statusLabel->setText("未连接"); return; }
        if (dll.reset()) {
            statusLabel->setText("复位成功");
        } else {
            statusLabel->setText("复位失败: " + MazakFrameBuilder::errorToString(dll.lastError()));
        }
    });

    return widget;
}
