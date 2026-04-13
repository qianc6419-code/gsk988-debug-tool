#include "protocol/knd/kndcommandwidget.h"
#include "protocol/knd/kndrestclient.h"
#include "protocol/knd/kndframebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QRegularExpression>

KndCommandWidget::KndCommandWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    auto& client = KndRestClient::instance();
    connect(&client, &KndRestClient::writeResultReceived,
        this, [this](const QString& op, bool ok, const QString& msg) {
            QString text = op + (ok ? " 成功" : " 失败: " + msg);
            // 更新所有 status label
            if (m_coordStatusLabel) m_coordStatusLabel->setText(text);
            if (m_macroStatusLabel) m_macroStatusLabel->setText(text);
            if (m_toolGeomStatusLabel) m_toolGeomStatusLabel->setText(text);
            if (m_toolWearStatusLabel) m_toolWearStatusLabel->setText(text);
            if (m_progStatusLabel) m_progStatusLabel->setText(text);
            if (m_plcStatusLabel) m_plcStatusLabel->setText(text);
            if (m_countStatusLabel) m_countStatusLabel->setText(text);
        });
}

void KndCommandWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    m_tabs = new QTabWidget;
    mainLayout->addWidget(m_tabs);

    m_tabs->addTab(createWorkCoordTab(), "工件坐标系");
    m_tabs->addTab(createMacroTab(), "宏变量");
    m_tabs->addTab(createToolTab(), "刀补");
    m_tabs->addTab(createProgramTab(), "程序管理");
    m_tabs->addTab(createPlcTab(), "PLC数据");
    m_tabs->addTab(createWorkCountTab(), "加工计数");
    m_tabs->addTab(createOperationTab(), "操作");
}

QWidget* KndCommandWidget::createWorkCoordTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;
    auto* readBtn = new QPushButton("读取");
    auto* writeBtn = new QPushButton("写入");
    ctrlLayout->addWidget(readBtn);
    ctrlLayout->addWidget(writeBtn);
    ctrlLayout->addStretch();
    layout->addLayout(ctrlLayout);

    m_coordTable = new QTableWidget(0, 4);
    m_coordTable->setHorizontalHeaderLabels({"坐标系", "X", "Y", "Z"});
    m_coordTable->horizontalHeader()->setStretchLastSection(true);
    m_coordTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_coordTable->setAlternatingRowColors(true);
    layout->addWidget(m_coordTable);

    m_coordStatusLabel = new QLabel;
    layout->addWidget(m_coordStatusLabel);

    auto& client = KndRestClient::instance();

    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_coordStatusLabel->setText("未连接"); return; }
        m_coordStatusLabel->setText("读取中...");
        c.getWorkCoordinates();
    });

    connect(&client, &KndRestClient::workCoordinatesReceived, this, [this](const QJsonObject& data) {
        auto wcs = KndFrameBuilder::parseWorkCoords(data);
        m_coordTable->setRowCount(wcs.size());
        for (int i = 0; i < wcs.size(); ++i) {
            auto setItem = [&](int col, const QString& text) {
                if (!m_coordTable->item(i, col)) {
                    auto* item = new QTableWidgetItem;
                    item->setTextAlignment(Qt::AlignCenter);
                    m_coordTable->setItem(i, col, item);
                }
                m_coordTable->item(i, col)->setText(text);
            };
            setItem(0, wcs[i].name);
            setItem(1, QString::number(wcs[i].values.value("X", 0.0), 'f', 3));
            setItem(2, QString::number(wcs[i].values.value("Y", 0.0), 'f', 3));
            setItem(3, QString::number(wcs[i].values.value("Z", 0.0), 'f', 3));
        }
        m_coordStatusLabel->setText("读取成功");
    });

    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_coordStatusLabel->setText("未连接"); return; }
        for (int i = 0; i < m_coordTable->rowCount(); ++i) {
            QString name = m_coordTable->item(i, 0) ? m_coordTable->item(i, 0)->text() : "";
            if (name.isEmpty()) continue;
            QJsonObject vals;
            auto getVal = [&](int col) -> double {
                return m_coordTable->item(i, col) ? m_coordTable->item(i, col)->text().toDouble() : 0.0;
            };
            vals["X"] = getVal(1);
            vals["Y"] = getVal(2);
            vals["Z"] = getVal(3);
            c.setWorkCoordinate(name, vals);
        }
        m_coordStatusLabel->setText("写入中...");
    });

    return widget;
}

QWidget* KndCommandWidget::createMacroTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("变量(如 100,200,300-305):"));
    m_macroNamesEdit = new QLineEdit("100,200,300");
    ctrlLayout->addWidget(m_macroNamesEdit);
    auto* readBtn = new QPushButton("读取");
    auto* writeBtn = new QPushButton("写入");
    ctrlLayout->addWidget(readBtn);
    ctrlLayout->addWidget(writeBtn);
    layout->addLayout(ctrlLayout);

    m_macroTable = new QTableWidget(0, 2);
    m_macroTable->setHorizontalHeaderLabels({"变量", "值"});
    m_macroTable->horizontalHeader()->setStretchLastSection(true);
    m_macroTable->setAlternatingRowColors(true);
    layout->addWidget(m_macroTable);

    m_macroStatusLabel = new QLabel;
    layout->addWidget(m_macroStatusLabel);

    auto& client = KndRestClient::instance();

    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_macroStatusLabel->setText("未连接"); return; }
        m_macroStatusLabel->setText("读取中...");
        c.getMacros(m_macroNamesEdit->text());
    });

    connect(&client, &KndRestClient::macrosReceived, this, [this](const QJsonObject& data) {
        auto vars = KndFrameBuilder::parseMacros(data);
        m_macroTable->setRowCount(vars.size());
        for (int i = 0; i < vars.size(); ++i) {
            m_macroTable->setItem(i, 0, new QTableWidgetItem(vars[i].name));
            m_macroTable->setItem(i, 1, new QTableWidgetItem(QString::number(vars[i].value, 'f', 4)));
        }
        m_macroStatusLabel->setText("读取成功");
    });

    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_macroStatusLabel->setText("未连接"); return; }
        for (int i = 0; i < m_macroTable->rowCount(); ++i) {
            QString name = m_macroTable->item(i, 0) ? m_macroTable->item(i, 0)->text() : "";
            double val = m_macroTable->item(i, 1) ? m_macroTable->item(i, 1)->text().toDouble() : 0.0;
            if (!name.isEmpty()) c.setMacro(name, val);
        }
        m_macroStatusLabel->setText("写入中...");
    });

    return widget;
}

QWidget* KndCommandWidget::createToolTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    m_toolTabs = new QTabWidget;
    layout->addWidget(m_toolTabs);

    // === 形状子 Tab ===
    {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        auto* ctrl = new QHBoxLayout;
        ctrl->addWidget(new QLabel("起始号:"));
        m_toolGeomStartSpin = new QSpinBox; m_toolGeomStartSpin->setRange(0, 999); m_toolGeomStartSpin->setValue(1);
        ctrl->addWidget(m_toolGeomStartSpin);
        ctrl->addWidget(new QLabel("数量(<=32):"));
        m_toolGeomCountSpin = new QSpinBox; m_toolGeomCountSpin->setRange(1, 32); m_toolGeomCountSpin->setValue(10);
        ctrl->addWidget(m_toolGeomCountSpin);
        auto* readBtn = new QPushButton("读取");
        auto* writeBtn = new QPushButton("写入");
        ctrl->addWidget(readBtn);
        ctrl->addWidget(writeBtn);
        l->addLayout(ctrl);

        m_toolGeomTable = new QTableWidget(0, 4);
        m_toolGeomTable->setHorizontalHeaderLabels({"刀补号", "X", "Z", "R"});
        m_toolGeomTable->horizontalHeader()->setStretchLastSection(true);
        m_toolGeomTable->setAlternatingRowColors(true);
        l->addWidget(m_toolGeomTable);

        m_toolGeomStatusLabel = new QLabel;
        l->addWidget(m_toolGeomStatusLabel);

        auto& client = KndRestClient::instance();
        connect(readBtn, &QPushButton::clicked, this, [this]() {
            auto& c = KndRestClient::instance();
            if (!c.isConnected()) { m_toolGeomStatusLabel->setText("未连接"); return; }
            m_toolGeomStatusLabel->setText("读取中...");
            c.getToolGeomBatch(m_toolGeomStartSpin->value(), m_toolGeomCountSpin->value());
        });
        connect(&client, &KndRestClient::toolGeomBatchReceived, this, [this](const QJsonArray& data) {
            auto offsets = KndFrameBuilder::parseToolOffsets(data);
            m_toolGeomTable->setRowCount(offsets.size());
            for (int i = 0; i < offsets.size(); ++i) {
                m_toolGeomTable->setItem(i, 0, new QTableWidgetItem(QString::number(offsets[i].ofsNo)));
                int col = 1;
                for (auto it = offsets[i].values.begin(); it != offsets[i].values.end() && col < 4; ++it, ++col) {
                    m_toolGeomTable->setItem(i, col, new QTableWidgetItem(QString::number(it.value(), 'f', 3)));
                }
            }
            m_toolGeomStatusLabel->setText("读取成功");
        });
        connect(writeBtn, &QPushButton::clicked, this, [this]() {
            auto& c = KndRestClient::instance();
            if (!c.isConnected()) { m_toolGeomStatusLabel->setText("未连接"); return; }
            for (int i = 0; i < m_toolGeomTable->rowCount(); ++i) {
                int ofsNo = m_toolGeomTable->item(i, 0) ? m_toolGeomTable->item(i, 0)->text().toInt() : 0;
                QJsonObject vals;
                auto getVal = [&](int col, const QString& key) {
                    vals[key] = m_toolGeomTable->item(i, col) ? m_toolGeomTable->item(i, col)->text().toDouble() : 0.0;
                };
                getVal(1, "X"); getVal(2, "Z"); getVal(3, "R");
                c.setToolGeom(ofsNo, vals);
            }
            m_toolGeomStatusLabel->setText("写入中...");
        });

        m_toolTabs->addTab(w, "形状");
    }

    // === 磨损子 Tab ===
    {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        auto* ctrl = new QHBoxLayout;
        ctrl->addWidget(new QLabel("起始号:"));
        m_toolWearStartSpin = new QSpinBox; m_toolWearStartSpin->setRange(0, 999); m_toolWearStartSpin->setValue(1);
        ctrl->addWidget(m_toolWearStartSpin);
        ctrl->addWidget(new QLabel("数量(<=32):"));
        m_toolWearCountSpin = new QSpinBox; m_toolWearCountSpin->setRange(1, 32); m_toolWearCountSpin->setValue(10);
        ctrl->addWidget(m_toolWearCountSpin);
        auto* readBtn = new QPushButton("读取");
        auto* writeBtn = new QPushButton("写入");
        ctrl->addWidget(readBtn);
        ctrl->addWidget(writeBtn);
        l->addLayout(ctrl);

        m_toolWearTable = new QTableWidget(0, 4);
        m_toolWearTable->setHorizontalHeaderLabels({"刀补号", "X", "Z", "R"});
        m_toolWearTable->horizontalHeader()->setStretchLastSection(true);
        m_toolWearTable->setAlternatingRowColors(true);
        l->addWidget(m_toolWearTable);

        m_toolWearStatusLabel = new QLabel;
        l->addWidget(m_toolWearStatusLabel);

        auto& client = KndRestClient::instance();
        connect(readBtn, &QPushButton::clicked, this, [this]() {
            auto& c = KndRestClient::instance();
            if (!c.isConnected()) { m_toolWearStatusLabel->setText("未连接"); return; }
            m_toolWearStatusLabel->setText("读取中...");
            c.getToolWearBatch(m_toolWearStartSpin->value(), m_toolWearCountSpin->value());
        });
        connect(&client, &KndRestClient::toolWearBatchReceived, this, [this](const QJsonArray& data) {
            auto offsets = KndFrameBuilder::parseToolOffsets(data);
            m_toolWearTable->setRowCount(offsets.size());
            for (int i = 0; i < offsets.size(); ++i) {
                m_toolWearTable->setItem(i, 0, new QTableWidgetItem(QString::number(offsets[i].ofsNo)));
                int col = 1;
                for (auto it = offsets[i].values.begin(); it != offsets[i].values.end() && col < 4; ++it, ++col) {
                    m_toolWearTable->setItem(i, col, new QTableWidgetItem(QString::number(it.value(), 'f', 3)));
                }
            }
            m_toolWearStatusLabel->setText("读取成功");
        });
        connect(writeBtn, &QPushButton::clicked, this, [this]() {
            auto& c = KndRestClient::instance();
            if (!c.isConnected()) { m_toolWearStatusLabel->setText("未连接"); return; }
            for (int i = 0; i < m_toolWearTable->rowCount(); ++i) {
                int ofsNo = m_toolWearTable->item(i, 0) ? m_toolWearTable->item(i, 0)->text().toInt() : 0;
                QJsonObject vals;
                auto getVal = [&](int col, const QString& key) {
                    vals[key] = m_toolWearTable->item(i, col) ? m_toolWearTable->item(i, col)->text().toDouble() : 0.0;
                };
                getVal(1, "X"); getVal(2, "Z"); getVal(3, "R");
                c.setToolWear(ofsNo, vals);
            }
            m_toolWearStatusLabel->setText("写入中...");
        });

        m_toolTabs->addTab(w, "磨损");
    }

    return widget;
}

QWidget* KndCommandWidget::createProgramTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);

    auto* splitter = new QSplitter(Qt::Horizontal);

    QPushButton* listBtn = nullptr;
    QPushButton* setCurBtn = nullptr;
    QPushButton* readBtn = nullptr;
    QPushButton* writeBtn = nullptr;
    QPushButton* deleteBtn = nullptr;
    QPushButton* mdiBtn = nullptr;

    // 左侧: 程序列表
    {
        auto* leftWidget = new QWidget;
        auto* leftLayout = new QVBoxLayout(leftWidget);
        auto* ctrlLayout = new QHBoxLayout;
        listBtn = new QPushButton("刷新列表");
        setCurBtn = new QPushButton("设为当前");
        ctrlLayout->addWidget(listBtn);
        ctrlLayout->addWidget(setCurBtn);
        leftLayout->addLayout(ctrlLayout);

        m_progList = new QListWidget;
        leftLayout->addWidget(m_progList);

        splitter->addWidget(leftWidget);
    }

    // 右侧: 程序内容
    {
        auto* rightWidget = new QWidget;
        auto* rightLayout = new QVBoxLayout(rightWidget);
        auto* ctrlLayout = new QHBoxLayout;
        ctrlLayout->addWidget(new QLabel("程序号:"));
        m_progNoSpin = new QSpinBox; m_progNoSpin->setRange(1, 99999);
        ctrlLayout->addWidget(m_progNoSpin);
        readBtn = new QPushButton("读取");
        writeBtn = new QPushButton("写入");
        deleteBtn = new QPushButton("删除");
        mdiBtn = new QPushButton("MDI执行");
        ctrlLayout->addWidget(readBtn);
        ctrlLayout->addWidget(writeBtn);
        ctrlLayout->addWidget(deleteBtn);
        ctrlLayout->addWidget(mdiBtn);
        rightLayout->addLayout(ctrlLayout);

        m_progContentEdit = new QTextEdit;
        rightLayout->addWidget(m_progContentEdit);

        m_progStatusLabel = new QLabel;
        rightLayout->addWidget(m_progStatusLabel);

        splitter->addWidget(rightWidget);
    }

    splitter->setSizes({200, 500});
    layout->addWidget(splitter);

    auto& client = KndRestClient::instance();

    // 刷新列表
    connect(listBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_progStatusLabel->setText("未连接"); return; }
        m_progStatusLabel->setText("获取列表中...");
        c.getPrograms();
    });

    connect(&client, &KndRestClient::programsReceived, this, [this](const QJsonArray& data) {
        m_progList->clear();
        for (const auto& item : data) {
            QJsonObject obj = item.toObject();
            QString text = QString("O%1").arg(obj["no"].toInt());
            if (obj.contains("comment")) {
                text += " " + obj["comment"].toString();
            }
            m_progList->addItem(text);
        }
        m_progStatusLabel->setText(QString("共 %1 个程序").arg(data.size()));
    });

    // 双击列表项读取程序
    connect(m_progList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        Q_UNUSED(item);
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) return;
        QString text = m_progList->currentItem()->text();
        QRegularExpression re("O(\\d+)");
        auto match = re.match(text);
        if (match.hasMatch()) {
            int no = match.captured(1).toInt();
            m_progNoSpin->setValue(no);
            m_progStatusLabel->setText("读取中...");
            c.getProgram(no);
        }
    });

    // 读取程序内容
    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_progStatusLabel->setText("未连接"); return; }
        m_progStatusLabel->setText("读取中...");
        c.getProgram(m_progNoSpin->value());
    });

    connect(&client, &KndRestClient::programContentReceived, this,
        [this](int no, const QString& content) {
        m_progNoSpin->setValue(no);
        m_progContentEdit->setPlainText(content);
        m_progStatusLabel->setText(QString("程序 O%1 读取成功").arg(no));
    });

    // 写入程序
    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_progStatusLabel->setText("未连接"); return; }
        c.writeProgram(m_progNoSpin->value(), m_progContentEdit->toPlainText());
        m_progStatusLabel->setText("写入中...");
    });

    // 删除程序
    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_progStatusLabel->setText("未连接"); return; }
        int ret = QMessageBox::warning(this, "确认删除",
            QString("确定要删除程序 O%1 吗？").arg(m_progNoSpin->value()),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            c.deleteProgram(m_progNoSpin->value());
            m_progStatusLabel->setText("删除中...");
        }
    });

    // MDI 执行
    connect(mdiBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_progStatusLabel->setText("未连接"); return; }
        c.sendMdi(m_progContentEdit->toPlainText());
        m_progStatusLabel->setText("MDI 发送中...");
    });

    // 设为当前程序
    connect(setCurBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_progStatusLabel->setText("未连接"); return; }
        if (m_progList->currentItem()) {
            QString text = m_progList->currentItem()->text();
            QRegularExpression re("O(\\d+)");
            auto match = re.match(text);
            if (match.hasMatch()) {
                c.setCurrentProgram(match.captured(1).toInt());
                m_progStatusLabel->setText("设置当前程序中...");
            }
        }
    });

    return widget;
}

QWidget* KndCommandWidget::createPlcTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;

    ctrlLayout->addWidget(new QLabel("区域:"));
    m_plcRegionCombo = new QComboBox;
    m_plcRegionCombo->addItems(KndFrameBuilder::plcRegions());
    ctrlLayout->addWidget(m_plcRegionCombo);

    ctrlLayout->addWidget(new QLabel("偏移:"));
    m_plcOffsetSpin = new QSpinBox; m_plcOffsetSpin->setRange(0, 65535);
    ctrlLayout->addWidget(m_plcOffsetSpin);

    ctrlLayout->addWidget(new QLabel("类型:"));
    m_plcTypeCombo = new QComboBox;
    m_plcTypeCombo->addItems({"u8", "u16", "u32", "s8", "s16", "s32"});
    ctrlLayout->addWidget(m_plcTypeCombo);

    ctrlLayout->addWidget(new QLabel("数量:"));
    m_plcCountSpin = new QSpinBox; m_plcCountSpin->setRange(1, 256); m_plcCountSpin->setValue(10);
    ctrlLayout->addWidget(m_plcCountSpin);

    auto* readBtn = new QPushButton("读取");
    auto* writeBtn = new QPushButton("写入");
    ctrlLayout->addWidget(readBtn);
    ctrlLayout->addWidget(writeBtn);
    layout->addLayout(ctrlLayout);

    m_plcTable = new QTableWidget(0, 2);
    m_plcTable->setHorizontalHeaderLabels({"偏移", "值"});
    m_plcTable->horizontalHeader()->setStretchLastSection(true);
    m_plcTable->setAlternatingRowColors(true);
    layout->addWidget(m_plcTable);

    m_plcStatusLabel = new QLabel;
    layout->addWidget(m_plcStatusLabel);

    auto& client = KndRestClient::instance();

    connect(readBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_plcStatusLabel->setText("未连接"); return; }
        m_plcStatusLabel->setText("读取中...");
        c.getPlcRegion(m_plcRegionCombo->currentText(),
                        m_plcOffsetSpin->value(),
                        m_plcTypeCombo->currentText(),
                        m_plcCountSpin->value());
    });

    connect(&client, &KndRestClient::systemInfoReceived, this, [this](const QJsonObject& data) {
        // PLC 数据复用 systemInfoReceived 信号
        if (data.contains("data")) {
            QJsonArray arr = data["data"].toArray();
            m_plcTable->setRowCount(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                m_plcTable->setItem(i, 0, new QTableWidgetItem(
                    QString::number(m_plcOffsetSpin->value() + i)));
                m_plcTable->setItem(i, 1, new QTableWidgetItem(
                    QString::number(arr[i].toInt())));
            }
            m_plcStatusLabel->setText("读取成功");
        }
    });

    connect(writeBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_plcStatusLabel->setText("未连接"); return; }

        // R 区警告
        if (m_plcRegionCombo->currentText() == "R") {
            int ret = QMessageBox::warning(this, "警告",
                "R 区是 PLC 内部数据，写入可能导致设备异常！确定继续？",
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) return;
        }

        QJsonArray dataArr;
        for (int i = 0; i < m_plcTable->rowCount(); ++i) {
            dataArr.append(m_plcTable->item(i, 1) ? m_plcTable->item(i, 1)->text().toInt() : 0);
        }
        c.setPlcRegion(m_plcRegionCombo->currentText(),
                       m_plcOffsetSpin->value(),
                       m_plcTypeCombo->currentText(),
                       dataArr);
        m_plcStatusLabel->setText("写入中...");
    });

    return widget;
}

QWidget* KndCommandWidget::createWorkCountTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* ctrlLayout = new QHBoxLayout;

    ctrlLayout->addWidget(new QLabel("加工计数:"));
    m_countTotalSpin = new QSpinBox; m_countTotalSpin->setRange(0, 999999);
    ctrlLayout->addWidget(m_countTotalSpin);
    auto* readCountBtn = new QPushButton("读取");
    auto* setCountBtn = new QPushButton("设置");
    ctrlLayout->addWidget(readCountBtn);
    ctrlLayout->addWidget(setCountBtn);
    ctrlLayout->addStretch();
    layout->addLayout(ctrlLayout);

    auto* ctrlLayout2 = new QHBoxLayout;
    ctrlLayout2->addWidget(new QLabel("目标件数:"));
    m_countGoalSpin = new QSpinBox; m_countGoalSpin->setRange(0, 999999);
    ctrlLayout2->addWidget(m_countGoalSpin);
    auto* readGoalBtn = new QPushButton("读取");
    auto* setGoalBtn = new QPushButton("设置");
    ctrlLayout2->addWidget(readGoalBtn);
    ctrlLayout2->addWidget(setGoalBtn);
    ctrlLayout2->addStretch();
    layout->addLayout(ctrlLayout2);

    m_countStatusLabel = new QLabel;
    layout->addWidget(m_countStatusLabel);
    layout->addStretch();

    auto& client = KndRestClient::instance();

    connect(readCountBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_countStatusLabel->setText("未连接"); return; }
        c.getWorkCounts();
    });
    connect(&client, &KndRestClient::workCountsReceived, this, [this](const QJsonObject& data) {
        m_countTotalSpin->setValue(data["total"].toInt());
        m_countStatusLabel->setText("加工计数读取成功");
    });

    connect(setCountBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_countStatusLabel->setText("未连接"); return; }
        c.setWorkCountTotal(m_countTotalSpin->value());
        m_countStatusLabel->setText("设置加工计数中...");
    });

    connect(readGoalBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_countStatusLabel->setText("未连接"); return; }
        c.getWorkCountGoals();
    });
    connect(&client, &KndRestClient::workCountGoalsReceived, this, [this](const QJsonObject& data) {
        m_countGoalSpin->setValue(data["total"].toInt());
        m_countStatusLabel->setText("目标件数读取成功");
    });

    connect(setGoalBtn, &QPushButton::clicked, this, [this]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { m_countStatusLabel->setText("未连接"); return; }
        c.setWorkCountGoal(m_countGoalSpin->value());
        m_countStatusLabel->setText("设置目标件数中...");
    });

    return widget;
}

QWidget* KndCommandWidget::createOperationTab()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    auto* btnLayout = new QHBoxLayout;

    auto* clearRelBtn = new QPushButton("清零相对坐标");
    clearRelBtn->setMinimumHeight(40);
    btnLayout->addWidget(clearRelBtn);

    auto* clearTimeBtn = new QPushButton("清零加工时间");
    clearTimeBtn->setMinimumHeight(40);
    btnLayout->addWidget(clearTimeBtn);

    layout->addLayout(btnLayout);

    auto* statusLabel = new QLabel;
    layout->addWidget(statusLabel);
    layout->addStretch();

    auto& client = KndRestClient::instance();

    connect(clearRelBtn, &QPushButton::clicked, this, [statusLabel]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { statusLabel->setText("未连接"); return; }
        c.clearRelativeCoors();
        statusLabel->setText("清零相对坐标中...");
    });

    connect(clearTimeBtn, &QPushButton::clicked, this, [statusLabel]() {
        auto& c = KndRestClient::instance();
        if (!c.isConnected()) { statusLabel->setText("未连接"); return; }
        c.clearCycleTime();
        statusLabel->setText("清零加工时间中...");
    });

    return widget;
}