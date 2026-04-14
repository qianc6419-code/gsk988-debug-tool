#include "syntecparsewidget.h"
#include "syntecframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

SyntecParseWidget::SyntecParseWidget(QWidget* parent) : QWidget(parent) { setupUI(); }

void SyntecParseWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(QStringLiteral("输入Hex帧 (空格分隔):")));
    m_hexInput = new QTextEdit;
    m_hexInput->setMaximumHeight(60);
    m_hexInput->setFontFamily("Consolas");
    layout->addWidget(m_hexInput);

    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel(QStringLiteral("数据类型:")));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({
        QStringLiteral("自动"), QStringLiteral("运行状态"), QStringLiteral("操作模式"),
        QStringLiteral("报警"), QStringLiteral("急停"), QStringLiteral("时间(秒)"),
        QStringLiteral("工件数"), QStringLiteral("倍率(%)"), QStringLiteral("速度(RPM)"),
        QStringLiteral("坐标(int64)"), QStringLiteral("程序名"), QStringLiteral("小数位数"),
        QStringLiteral("原始HEX")
    });
    ctrlLayout->addWidget(m_typeCombo);
    m_parseBtn = new QPushButton(QStringLiteral("解析"));
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &SyntecParseWidget::doParse);
    ctrlLayout->addWidget(m_parseBtn);
    ctrlLayout->addStretch();
    layout->addLayout(ctrlLayout);

    layout->addWidget(new QLabel(QStringLiteral("解析结果:")));
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");
    layout->addWidget(m_resultDisplay, 1);
}

void SyntecParseWidget::doParse()
{
    using FB = SyntecFrameBuilder;

    QString hexStr = m_hexInput->toPlainText().trimmed();
    hexStr.remove(' ');
    hexStr.remove(',');
    hexStr.remove("0x", Qt::CaseInsensitive);
    QByteArray frame = QByteArray::fromHex(hexStr.toUtf8());

    if (frame.size() < 4) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧太短</span>");
        return;
    }

    qint32 totalLen = 0;
    memcpy(&totalLen, frame.constData(), 4);
    quint8 tid = FB::extractTid(frame);
    qint32 err1 = FB::extractInt32(frame, 12);
    qint32 err2 = FB::extractInt32(frame, 16);

    QString html;
    html += QStringLiteral("<b>帧长度:</b> %1 字节<br>").arg(frame.size());
    html += QStringLiteral("<b>数据长度:</b> %1<br>").arg(totalLen);
    html += QStringLiteral("<b>TID:</b> %1<br>").arg(tid);
    html += QStringLiteral("<b>错误码1:</b> %1, <b>错误码2:</b> %2<br>").arg(err1).arg(err2);

    if (err1 != 0 || err2 != 0) {
        html += QStringLiteral("<span style='color:red;'>帧包含错误!</span><br>");
    }

    if (frame.size() < 20) {
        html += QStringLiteral("<span style='color:orange;'>帧不包含数据区</span>");
        m_resultDisplay->setHtml(html);
        return;
    }

    QByteArray data = frame.mid(20);
    int type = m_typeCombo->currentIndex();

    if (type == 0) {
        QStringList hexList;
        for (int i = 0; i < data.size(); ++i)
            hexList << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        html += QStringLiteral("<br><b>数据区 HEX:</b> %1<br>").arg(hexList.join(" "));
        if (data.size() >= 4) html += QStringLiteral("<b>Int32[0]:</b> %1<br>").arg(FB::extractInt32(data, 0));
        if (data.size() >= 8) html += QStringLiteral("<b>Int32[4]:</b> %1<br>").arg(FB::extractInt32(data, 4));
        if (data.size() >= 12) html += QStringLiteral("<b>Int32[8]:</b> %1<br>").arg(FB::extractInt32(data, 8));
    } else if (type == 1) {
        html += QStringLiteral("<b>运行状态:</b> %1 (%2)").arg(FB::statusToString(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    } else if (type == 2) {
        html += QStringLiteral("<b>操作模式:</b> %1 (%2)").arg(FB::modeToString(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    } else if (type == 3) {
        html += QStringLiteral("<b>报警数量:</b> %1").arg(FB::extractInt32(data, 0));
    } else if (type == 4) {
        if (data.size() >= 5) {
            int emg = static_cast<quint8>(data[4]);
            html += QStringLiteral("<b>急停:</b> %1 (0x%2)").arg(FB::emgToString(emg)).arg(emg, 2, 16, QChar('0'));
        }
    } else if (type == 5) {
        html += QStringLiteral("<b>时间:</b> %1 (%2秒)").arg(FB::formatTime(FB::extractInt32(data, 0))).arg(FB::extractInt32(data, 0));
    } else if (type == 6) {
        html += QStringLiteral("<b>工件数:</b> %1").arg(FB::extractInt32(data, 0));
    } else if (type == 7) {
        html += QStringLiteral("<b>倍率:</b> %1%").arg(FB::extractInt32(data, 0));
    } else if (type == 8) {
        html += QStringLiteral("<b>速度:</b> %1 RPM").arg(FB::extractInt32(data, 0));
    } else if (type == 9) {
        for (int i = 0; i < 3 && i * 8 + 8 <= data.size(); ++i) {
            qint64 raw = FB::extractInt64(data, i * 8);
            char axis = 'X' + i;
            html += QStringLiteral("<b>%1:</b> %2 (raw)<br>").arg(axis).arg(raw);
        }
    } else if (type == 10) {
        html += QStringLiteral("<b>程序名:</b> %1").arg(FB::extractString(data, 0));
    } else if (type == 11) {
        html += QStringLiteral("<b>小数位数:</b> %1").arg(FB::extractInt32(data, 0));
    } else {
        QStringList hexList;
        for (int i = 0; i < data.size(); ++i)
            hexList << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        html += QStringLiteral("<b>HEX:</b> %1").arg(hexList.join(" "));
    }

    m_resultDisplay->setHtml(html);
}
