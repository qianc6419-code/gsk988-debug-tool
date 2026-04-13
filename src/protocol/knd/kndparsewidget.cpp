#include "protocol/knd/kndparsewidget.h"
#include "protocol/knd/kndframebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

KndParseWidget::KndParseWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("数据类型:"));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({
        "系统状态",
        "坐标",
        "报警",
        "倍率",
        "主轴速率",
        "加工时间",
        "工件坐标系",
        "宏变量",
        "刀补",
        "G代码模态",
        "程序列表",
        "PLC数据",
    });
    ctrlLayout->addWidget(m_typeCombo);
    auto* parseBtn = new QPushButton("解析");
    ctrlLayout->addWidget(parseBtn);
    ctrlLayout->addStretch();
    mainLayout->addLayout(ctrlLayout);

    m_inputEdit = new QTextEdit;
    m_inputEdit->setPlaceholderText("粘贴 KND API 的 JSON 响应文本...");
    m_inputEdit->setMaximumHeight(150);
    mainLayout->addWidget(m_inputEdit);

    m_resultEdit = new QTextEdit;
    m_resultEdit->setReadOnly(true);
    m_resultEdit->setPlaceholderText("解析结果");
    mainLayout->addWidget(m_resultEdit);

    connect(parseBtn, &QPushButton::clicked, this, &KndParseWidget::onParse);
}

void KndParseWidget::onParse()
{
    QString input = m_inputEdit->toPlainText().trimmed();
    if (input.isEmpty()) {
        m_resultEdit->setText("请输入 JSON 数据");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8());
    if (doc.isNull()) {
        m_resultEdit->setText("JSON 解析失败，请检查格式");
        return;
    }

    QString result;
    int type = m_typeCombo->currentIndex();

    switch (type) {
    case 0: { // 系统状态
        QJsonObject obj = doc.object();
        result = QString("运行状态: %1 (%2)\n"
            "工作模式: %3 (%4)\n"
            "报警号: %5\n"
            "执行程序: O%6")
            .arg(KndFrameBuilder::runStatusToString(obj["run-status"].toInt()))
            .arg(obj["run-status"].toInt())
            .arg(KndFrameBuilder::oprModeToString(obj["opr-mode"].toInt()))
            .arg(obj["opr-mode"].toInt())
            .arg(obj["alarm"].toInt())
            .arg(obj["exec-no"].toInt());
        break;
    }
    case 1: { // 坐标
        auto coords = KndFrameBuilder::parseCoordinates(doc.object());
        result = QString("共 %1 个轴:\n").arg(coords.size());
        for (const auto& c : coords) {
            result += QString("  %1: 机床=%2  绝对=%3  相对=%4  余移动=%5\n")
                .arg(c.name)
                .arg(c.machine, 0, 'f', 3)
                .arg(c.absolute, 0, 'f', 3)
                .arg(c.relative, 0, 'f', 3)
                .arg(c.distanceToGo, 0, 'f', 3);
        }
        break;
    }
    case 2: { // 报警
        auto alarms = KndFrameBuilder::parseAlarms(doc.object());
        if (alarms.isEmpty()) {
            result = "无报警";
        } else {
            for (const auto& a : alarms) {
                result += QString("报警 %1: %2\n").arg(a.no).arg(a.message);
            }
        }
        break;
    }
    case 3: { // 倍率
        int ov = KndFrameBuilder::parseOverride(doc.object());
        result = QString("倍率: %1%").arg(ov);
        break;
    }
    case 4: { // 主轴速率
        auto speeds = KndFrameBuilder::parseSpindleSpeeds(doc.object());
        for (const auto& s : speeds) {
            result += QString("主轴%1: 设定=%2  实际=%3\n")
                .arg(s.spNo)
                .arg(s.setSpeed, 0, 'f', 0)
                .arg(s.actSpeed, 0, 'f', 0);
        }
        break;
    }
    case 5: { // 加工时间
        auto ct = KndFrameBuilder::parseCycleTime(doc.object());
        result = QString("循环时间: %1:%2:%3\n加工时间: %4:%5:%6")
            .arg(ct.cycleHour, 2, 10, QChar('0'))
            .arg(ct.cycleMin, 2, 10, QChar('0'))
            .arg(ct.cycleSec, 2, 10, QChar('0'))
            .arg(ct.totalHour, 2, 10, QChar('0'))
            .arg(ct.totalMin, 2, 10, QChar('0'))
            .arg(ct.totalSec, 2, 10, QChar('0'));
        break;
    }
    case 6: { // 工件坐标系
        auto wcs = KndFrameBuilder::parseWorkCoords(doc.object());
        for (const auto& w : wcs) {
            result += w.name + ": ";
            for (auto it = w.values.begin(); it != w.values.end(); ++it) {
                if (it != w.values.begin()) result += "  ";
                result += QString("%1=%2").arg(it.key()).arg(it.value(), 0, 'f', 3);
            }
            result += "\n";
        }
        break;
    }
    case 7: { // 宏变量
        auto vars = KndFrameBuilder::parseMacros(doc.object());
        for (const auto& v : vars) {
            result += QString("%1 = %2\n").arg(v.name).arg(v.value, 0, 'f', 4);
        }
        break;
    }
    case 8: { // 刀补
        if (doc.isArray()) {
            auto offsets = KndFrameBuilder::parseToolOffsets(doc.array());
            for (const auto& o : offsets) {
                result += QString("刀补%1: ").arg(o.ofsNo);
                for (auto it = o.values.begin(); it != o.values.end(); ++it) {
                    if (it != o.values.begin()) result += "  ";
                    result += QString("%1=%2").arg(it.key()).arg(it.value(), 0, 'f', 3);
                }
                result += "\n";
            }
        }
        break;
    }
    case 9: { // G代码模态
        result = QString::fromUtf8(QJsonDocument(doc.object()).toJson(QJsonDocument::Indented));
        break;
    }
    case 10: { // 程序列表
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            result = QString("共 %1 个程序:\n").arg(arr.size());
            for (const auto& item : arr) {
                QJsonObject obj = item.toObject();
                result += QString("O%1").arg(obj["no"].toInt());
                if (obj.contains("comment")) {
                    result += " " + obj["comment"].toString();
                }
                result += "\n";
            }
        }
        break;
    }
    case 11: { // PLC数据
        result = QString::fromUtf8(QJsonDocument(doc.object()).toJson(QJsonDocument::Indented));
        break;
    }
    default:
        result = QString::fromUtf8(QJsonDocument(doc.object()).toJson(QJsonDocument::Indented));
    }

    m_resultEdit->setText(result);
}