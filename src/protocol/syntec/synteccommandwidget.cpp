#include "synteccommandwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>

SyntecCommandWidget::SyntecCommandWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SyntecCommandWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    auto* tabs = new QTabWidget;
    layout->addWidget(tabs);

    // === Tab 1: 状态查询 ===
    {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);

        auto* ctrlLayout = new QHBoxLayout;
        ctrlLayout->addWidget(new QLabel(QStringLiteral("命令:")));
        m_cmdCombo = new QComboBox;
        m_cmdCombo->addItem(QStringLiteral("程序名 (0x01)"), 0x01);
        m_cmdCombo->addItem(QStringLiteral("操作模式 (0x02)"), 0x02);
        m_cmdCombo->addItem(QStringLiteral("运行状态 (0x03)"), 0x03);
        m_cmdCombo->addItem(QStringLiteral("报警数量 (0x04)"), 0x04);
        m_cmdCombo->addItem(QStringLiteral("急停状态 (0x05)"), 0x05);
        m_cmdCombo->addItem(QStringLiteral("开机时间 (0x06)"), 0x06);
        m_cmdCombo->addItem(QStringLiteral("切削时间 (0x07)"), 0x07);
        m_cmdCombo->addItem(QStringLiteral("循环时间 (0x08)"), 0x08);
        m_cmdCombo->addItem(QStringLiteral("工件数 (0x09)"), 0x09);
        m_cmdCombo->addItem(QStringLiteral("需求工件数 (0x0A)"), 0x0A);
        m_cmdCombo->addItem(QStringLiteral("总工件数 (0x0B)"), 0x0B);
        m_cmdCombo->addItem(QStringLiteral("进给倍率 (0x0C)"), 0x0C);
        m_cmdCombo->addItem(QStringLiteral("主轴倍率 (0x0D)"), 0x0D);
        m_cmdCombo->addItem(QStringLiteral("主轴速度 (0x0E)"), 0x0E);
        m_cmdCombo->addItem(QStringLiteral("进给速度 (0x0F)"), 0x0F);
        m_cmdCombo->addItem(QStringLiteral("小数位数 (0x10)"), 0x10);
        m_cmdCombo->addItem(QStringLiteral("相对坐标 (0x20)"), 0x20);
        m_cmdCombo->addItem(QStringLiteral("机床坐标 (0x21)"), 0x21);
        ctrlLayout->addWidget(m_cmdCombo);

        m_sendBtn = new QPushButton(QStringLiteral("查询"));
        m_sendBtn->setFixedWidth(80);
        ctrlLayout->addWidget(m_sendBtn);
        ctrlLayout->addStretch();
        l->addLayout(ctrlLayout);

        m_resultDisplay = new QTextEdit;
        m_resultDisplay->setReadOnly(true);
        m_resultDisplay->setFontFamily("Consolas");
        m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");
        l->addWidget(m_resultDisplay);

        connect(m_sendBtn, &QPushButton::clicked, this, [this]() {
            int idx = m_cmdCombo->currentIndex();
            if (idx < 0) return;
            quint8 cmdCode = static_cast<quint8>(m_cmdCombo->itemData(idx).toInt());
            m_resultDisplay->clear();
            emit sendCommand(cmdCode, QByteArray());
        });

        tabs->addTab(w, QStringLiteral("状态查询"));
    }

    // === Tab 2: 操作 ===
    {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        l->addWidget(new QLabel(QStringLiteral("新代协议暂不支持写入命令。\n后续有写入协议文档可扩展。")));
        l->addStretch();
        tabs->addTab(w, QStringLiteral("操作"));
    }
}

void SyntecCommandWidget::showResponse(const ParsedResponse& resp, const QString& interpretation)
{
    QStringList hex;
    for (int i = 0; i < resp.rawData.size(); ++i)
        hex << QString("%1").arg(static_cast<quint8>(resp.rawData[i]), 2, 16, QChar('0')).toUpper();

    m_resultDisplay->append(QStringLiteral("=== 响应 ==="));
    m_resultDisplay->append(QStringLiteral("状态: %1").arg(resp.isValid ? QStringLiteral("成功") : QStringLiteral("失败")));
    m_resultDisplay->append(QStringLiteral("解析: %1").arg(interpretation));
    m_resultDisplay->append(QStringLiteral("HEX: %1").arg(hex.join(" ")));
}
