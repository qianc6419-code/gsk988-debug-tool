#include "protocol/mazak/mazakparsewidget.h"
#include "protocol/mazak/mazakframebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

MazakParseWidget::MazakParseWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("数据类型:"));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({
        "MTC_ONE_RUNNING_INFO",
        "MTC_ONE_AXIS_INFO",
        "MTC_ONE_SPINDLE_INFO",
        "MAZ_ALARM",
        "MAZ_PROINFO",
        "MAZ_TOOLINFO",
        "MAZ_TOFFCOMP",
        "MAZ_NCPOS",
    });
    ctrlLayout->addWidget(m_typeCombo);
    auto* parseBtn = new QPushButton("解析");
    ctrlLayout->addWidget(parseBtn);
    ctrlLayout->addStretch();
    mainLayout->addLayout(ctrlLayout);

    m_inputEdit = new QTextEdit;
    m_inputEdit->setPlaceholderText("输入十六进制数据 (如: 00 01 02 03 ...)");
    m_inputEdit->setMaximumHeight(150);
    mainLayout->addWidget(m_inputEdit);

    m_resultEdit = new QTextEdit;
    m_resultEdit->setReadOnly(true);
    m_resultEdit->setPlaceholderText("解析结果");
    mainLayout->addWidget(m_resultEdit);

    connect(parseBtn, &QPushButton::clicked, this, &MazakParseWidget::onParse);
}

void MazakParseWidget::onParse()
{
    QString hexStr = m_inputEdit->toPlainText().simplified();
    hexStr.remove(' ');
    if (hexStr.isEmpty()) {
        m_resultEdit->setText("请输入十六进制数据");
        return;
    }

    QByteArray data = QByteArray::fromHex(hexStr.toUtf8());
    if (data.isEmpty()) {
        m_resultEdit->setText("十六进制数据格式错误");
        return;
    }

    QString result;
    int type = m_typeCombo->currentIndex();

    switch (type) {
    case 0: { // MTC_ONE_RUNNING_INFO
        if (data.size() < static_cast<int>(sizeof(MTC_ONE_RUNNING_INFO))) {
            result = "数据不足，需要 " + QString::number(sizeof(MTC_ONE_RUNNING_INFO)) + " 字节";
            break;
        }
        auto* p = reinterpret_cast<const MTC_ONE_RUNNING_INFO*>(data.constData());
        result = QString("sts: %1\nncmode: %2\nncsts: %3\ntno: %4\n"
            "sp_override: %5\nax_override: %6\nrpid_override: %7\n"
            "almno: %8\nprtcnt: %9")
            .arg(p->sts).arg(p->ncmode).arg(p->ncsts).arg(p->tno)
            .arg(p->sp_override).arg(p->ax_override).arg(p->rpid_override)
            .arg(p->almno).arg(p->prtcnt);
        break;
    }
    case 1: { // MTC_ONE_AXIS_INFO
        if (data.size() < static_cast<int>(sizeof(MTC_ONE_AXIS_INFO))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MTC_ONE_AXIS_INFO*>(data.constData());
        result = QString("axis_name: %1\npos: %2\nmc_pos: %3\nload: %4\nfeed: %5")
            .arg(MazakFrameBuilder::fixedString(p->axis_name, 4))
            .arg(p->pos, 0, 'f', 3)
            .arg(p->mc_pos, 0, 'f', 3)
            .arg(p->load, 0, 'f', 1)
            .arg(p->feed, 0, 'f', 3);
        break;
    }
    case 2: { // MTC_ONE_SPINDLE_INFO
        if (data.size() < static_cast<int>(sizeof(MTC_ONE_SPINDLE_INFO))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MTC_ONE_SPINDLE_INFO*>(data.constData());
        result = QString("sts: %1\ntype: %2\nrot: %3\nload: %4\ntemp: %5")
            .arg(p->sts).arg(p->type)
            .arg(p->rot, 0, 'f', 1)
            .arg(p->load, 0, 'f', 1)
            .arg(p->temp, 0, 'f', 1);
        break;
    }
    case 3: { // MAZ_ALARM
        if (data.size() < static_cast<int>(sizeof(MAZ_ALARM))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MAZ_ALARM*>(data.constData());
        result = QString("eno: %1\nmessage: %2\npa1: %3")
            .arg(p->eno)
            .arg(MazakFrameBuilder::fixedString(p->message, 64))
            .arg(MazakFrameBuilder::fixedString(p->pa1, 32));
        break;
    }
    case 4: { // MAZ_PROINFO
        if (data.size() < static_cast<int>(sizeof(MAZ_PROINFO))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MAZ_PROINFO*>(data.constData());
        result = QString("wno: %1\ncomment: %2\ntype: %3")
            .arg(MazakFrameBuilder::fixedString(p->wno, 33))
            .arg(MazakFrameBuilder::fixedString(p->comment, 49))
            .arg(p->type);
        break;
    }
    case 5: { // MAZ_TOOLINFO
        if (data.size() < static_cast<int>(sizeof(MAZ_TOOLINFO))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MAZ_TOOLINFO*>(data.constData());
        result = QString("tno: %1\nsuf: %2\nsts: %3").arg(p->tno).arg(p->suf).arg(p->sts);
        break;
    }
    case 6: { // MAZ_TOFFCOMP
        if (data.size() < static_cast<int>(sizeof(MAZ_TOFFCOMP))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MAZ_TOFFCOMP*>(data.constData());
        result = QString("type: %1\noffset1: %2\noffset2: %3\noffset3: %4")
            .arg(p->type).arg(p->offset1).arg(p->offset2).arg(p->offset3);
        break;
    }
    case 7: { // MAZ_NCPOS
        if (data.size() < static_cast<int>(sizeof(MAZ_NCPOS))) {
            result = "数据不足";
            break;
        }
        auto* p = reinterpret_cast<const MAZ_NCPOS*>(data.constData());
        result = QString("status: %1\n").arg(MazakFrameBuilder::fixedString(p->status, 16));
        for (int i = 0; i < 16 && p->data[i] != 0; ++i) {
            result += QString("data[%1]: %2 (%3)\n")
                .arg(i).arg(p->data[i]).arg(p->data[i] / 10000.0, 0, 'f', 3);
        }
        break;
    }
    default:
        result = "未知类型";
    }

    m_resultEdit->setText(result);
}
