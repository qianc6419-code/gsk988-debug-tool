#include "fanucparsewidget.h"
#include "fanucframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

FanucParseWidget::FanucParseWidget(QWidget* parent) : QWidget(parent) { setupUI(); }

void FanucParseWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    // Input area
    layout->addWidget(new QLabel("输入Hex帧 (空格分隔):"));
    m_hexInput = new QTextEdit;
    m_hexInput->setMaximumHeight(60);
    m_hexInput->setFontFamily("Consolas");
    layout->addWidget(m_hexInput);

    // Parse button
    auto* btnBar = new QHBoxLayout;
    m_parseBtn = new QPushButton("解析");
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &FanucParseWidget::doParse);
    btnBar->addWidget(m_parseBtn);
    btnBar->addStretch();
    layout->addLayout(btnBar);

    // Result
    layout->addWidget(new QLabel("解析结果:"));
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");
    layout->addWidget(m_resultDisplay, 1);
}

void FanucParseWidget::doParse()
{
    QString hexStr = m_hexInput->toPlainText().trimmed();
    hexStr.remove(' ');
    hexStr.remove(',');
    hexStr.remove("0x", Qt::CaseInsensitive);
    QByteArray frame = QByteArray::fromHex(hexStr.toUtf8());

    if (frame.size() < 10) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧太短 (最少10字节)</span>");
        return;
    }

    QString html;
    html += "<b>帧长度:</b> " + QString::number(frame.size()) + " 字节<br>";

    // Header
    bool headerOk = FanucFrameBuilder::validateHeader(frame);
    html += QString("<b>协议头:</b> ") + (headerOk ?
        QString("<span style='color:green;'>A0 A0 A0 A0 (正确)</span>") :
        QString("<span style='color:red;'>错误</span>")) + "<br>";

    // Function code
    if (frame.size() >= 8) {
        quint8 f1 = static_cast<quint8>(frame[4]);
        quint8 f2 = static_cast<quint8>(frame[5]);
        quint8 f3 = static_cast<quint8>(frame[6]);
        quint8 f4 = static_cast<quint8>(frame[7]);
        html += QString("<b>功能码:</b> %1 %2 %3 %4")
            .arg(f1, 2, 16, QChar('0')).toUpper()
            .arg(f2, 2, 16, QChar('0')).toUpper()
            .arg(f3, 2, 16, QChar('0')).toUpper()
            .arg(f4, 2, 16, QChar('0')).toUpper();
        if (f3 == 0x21 && f4 == 0x01) html += " (请求)";
        else if (f3 == 0x21 && f4 == 0x02) html += " (响应)";
        html += "<br>";
    }

    // Data length
    quint16 dataLen = FanucFrameBuilder::getDataLength(frame);
    html += "<b>数据长度:</b> " + QString::number(dataLen) + "<br>";

    // Data hex dump
    if (frame.size() > 10) {
        QByteArray data = frame.mid(10);
        QStringList hexDump;
        for (int i = 0; i < data.size(); ++i)
            hexDump << QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        html += "<b>数据区:</b> " + hexDump.join(" ");
    }

    // Try to parse block count
    if (frame.size() >= 12) {
        qint16 blockCount = FanucFrameBuilder::decodeInt16(frame, 10);
        html += "<br><b>数据块个数:</b> " + QString::number(blockCount);

        // Parse each block
        int offset = 12;
        for (int b = 0; b < blockCount && offset + 28 <= frame.size(); ++b) {
            qint16 blockLen = FanucFrameBuilder::decodeInt16(frame, offset);
            qint16 ncFlag = FanucFrameBuilder::decodeInt16(frame, offset + 2);
            qint16 blockFunc = FanucFrameBuilder::decodeInt16(frame, offset + 6);

            html += QString("<br>&nbsp;&nbsp;<b>块%1:</b> 长度=%2, NC/PMC=%3, 功能码=0x%4")
                .arg(b + 1)
                .arg(blockLen)
                .arg(ncFlag)
                .arg(blockFunc, 4, 16, QChar('0')).toUpper();

            offset += blockLen;
        }
    }

    m_resultDisplay->setHtml(html);
}