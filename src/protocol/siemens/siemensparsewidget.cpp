#include "siemensparsewidget.h"
#include "siemensframebuilder.h"
#include "siemensprotocol.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

SiemensParseWidget::SiemensParseWidget(QWidget* parent) : QWidget(parent) { setupUI(); }

void SiemensParseWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    // Input area
    layout->addWidget(new QLabel(QStringLiteral("输入Hex帧 (空格分隔):")));
    m_hexInput = new QTextEdit;
    m_hexInput->setMaximumHeight(60);
    m_hexInput->setFontFamily("Consolas");
    layout->addWidget(m_hexInput);

    // Parse button
    auto* btnBar = new QHBoxLayout;
    m_parseBtn = new QPushButton(QStringLiteral("解析"));
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &SiemensParseWidget::doParse);
    btnBar->addWidget(m_parseBtn);
    btnBar->addStretch();
    layout->addLayout(btnBar);

    // Result
    layout->addWidget(new QLabel(QStringLiteral("解析结果:")));
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 12px; background: #f5f5f5; }");
    layout->addWidget(m_resultDisplay, 1);
}

void SiemensParseWidget::doParse()
{
    QString hexStr = m_hexInput->toPlainText().trimmed();
    hexStr.remove(' ');
    hexStr.remove(',');
    hexStr.remove("0x", Qt::CaseInsensitive);
    QByteArray frame = QByteArray::fromHex(hexStr.toUtf8());

    if (frame.size() < 4) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧太短 (最少4字节TPKT头)</span>");
        return;
    }

    QString html;
    html += "<b>帧长度:</b> " + QString::number(frame.size()) + " 字节<br>";

    // TPKT Header
    quint8 tpktVersion = static_cast<quint8>(frame[0]);
    quint16 totalLen = (static_cast<quint8>(frame[2]) << 8) |
                        static_cast<quint8>(frame[3]);
    bool tpktOk = (tpktVersion == 0x03);
    html += QString("<b>TPKT头:</b> 03 00 %1 %2 ")
        .arg(static_cast<quint8>(frame[2]), 2, 16, QChar('0')).toUpper()
        .arg(static_cast<quint8>(frame[3]), 2, 16, QChar('0')).toUpper();
    html += tpktOk ?
        QString("<span style='color:green;'>(协议版本正确)</span>") :
        QString("<span style='color:red;'>(协议版本错误, 期望 0x03)</span>");
    html += "<br>";
    html += QString("<b>TPKT总长度:</b> %1").arg(totalLen);
    if (frame.size() < totalLen)
        html += QString(" <span style='color:red;'>不完整 (需要%1字节, 实际%2)</span>")
                      .arg(totalLen).arg(frame.size());
    else if (frame.size() > totalLen)
        html += QString(" <span style='color:orange;'>有多余数据 (%1字节)</span>")
                      .arg(frame.size() - totalLen);
    html += "<br>";

    if (frame.size() >= 6) {
        // COTP
        quint8 cotpLI = static_cast<quint8>(frame[4]);
        quint8 cotpType = static_cast<quint8>(frame[5]);
        html += QString("<b>COTP:</b> LI=%1, Type=0x%2")
            .arg(cotpLI)
            .arg(cotpType, 2, 16, QChar('0')).toUpper();

        if (cotpType == 0xE0) html += " (连接请求)";
        else if (cotpType == 0xD0) html += " (连接确认)";
        else if (cotpType == 0xF0) html += " (数据传输)";
        html += "<br>";

        // S7Comm
        if (frame.size() >= 8 && cotpType == 0xF0 &&
            static_cast<quint8>(frame[7]) == 0x32) {

            quint8 rosctr = static_cast<quint8>(frame[8]);
            html += QString("<b>S7协议:</b> 0x32 ");
            html += (rosctr == 0x01) ? "<span style='color:#2196F3;'>(Job/请求)</span>" :
                     (rosctr == 0x02) ? "<span style='color:#FF9800;'>(Ack)</span>" :
                     (rosctr == 0x03) ? "<span style='color:#4CAF50;'>(Ack-Data/响应)</span>" :
                     "(未知)";
            html += "<br>";

            if (frame.size() >= 18) {
                quint16 paramLen = (static_cast<quint8>(frame[13]) << 8) |
                                    static_cast<quint8>(frame[14]);
                quint16 dataLen = (static_cast<quint8>(frame[15]) << 8) |
                                   static_cast<quint8>(frame[16]);
                html += QString("<b>参数长度:</b> %1, <b>数据长度:</b> %2<br>")
                    .arg(paramLen).arg(dataLen);

                quint8 func = static_cast<quint8>(frame[18]);
                html += QString("<b>功能码:</b> 0x%1")
                    .arg(func, 2, 16, QChar('0')).toUpper();
                if (func == 0x04) html += " (Read Var)";
                else if (func == 0x05) html += " (Write Var)";
                else if (func == 0x00) html += " (Setup)";
                html += "<br>";
            }

            // Item count
            if (frame.size() >= 20) {
                quint8 itemCount = static_cast<quint8>(frame[19]);
                html += QString("<b>数据项:</b> %1 个<br>").arg(itemCount);

                // Parse each S7ANY item
                int offset = 20;
                for (int i = 0; i < itemCount && offset + 12 <= frame.size(); ++i) {
                    quint8 varSpec = static_cast<quint8>(frame[offset]);
                    quint8 addrLen = static_cast<quint8>(frame[offset + 1]);
                    if (varSpec != 0x12 || addrLen < 4) break;

                    quint8 syntaxId = static_cast<quint8>(frame[offset + 2]);
                    quint8 transport = static_cast<quint8>(frame[offset + 3]);
                    quint16 numElem = (static_cast<quint8>(frame[offset + 4]) << 8) |
                                      static_cast<quint8>(frame[offset + 5]);
                    quint16 dbNum = (static_cast<quint8>(frame[offset + 6]) << 8) |
                                     static_cast<quint8>(frame[offset + 7]);
                    quint8 area = static_cast<quint8>(frame[offset + 8]);

                    html += QString("<br>&nbsp;&nbsp;<b>项%1:</b> ")
                        .arg(i + 1);
                    html += QString("Syntax=0x%1, Transport=0x%2, 元素数=%3, DB=%4, Area=0x%5")
                        .arg(syntaxId, 2, 16, QChar('0')).toUpper()
                        .arg(transport, 2, 16, QChar('0')).toUpper()
                        .arg(numElem)
                        .arg(dbNum)
                        .arg(area, 2, 16, QChar('0')).toUpper();

                    if (addrLen >= 10) {
                        quint32 addr = (static_cast<quint8>(frame[offset + 9]) << 16) |
                                        (static_cast<quint8>(frame[offset + 10]) << 8) |
                                        static_cast<quint8>(frame[offset + 11]);
                        html += QString(", 地址=0x%1").arg(addr, 6, 16, QChar('0')).toUpper();
                    }
                    html += "<br>";

                    offset += 2 + addrLen;
                }
            }

            // Data area (offset 25+) for responses
            if (rosctr == 0x03 && frame.size() > 25) {
                html += "<br><b>=== 响应数据区 (offset 25) ===</b><br>";
                QByteArray dataArea = frame.mid(25);
                QStringList dataHex;
                for (int i = 0; i < dataArea.size(); ++i)
                    dataHex << QString("%1").arg(static_cast<quint8>(dataArea[i]), 2, 16, QChar('0')).toUpper();
                html += "&nbsp;&nbsp;<b>HEX:</b> " + dataHex.join(" ") + "<br>";

                // Try parsing as common data types
                if (dataArea.size() >= 4) {
                    float fval = SiemensFrameBuilder::parseFloat(frame);
                    html += QString("&nbsp;&nbsp;<b>Float:</b> %1<br>").arg(fval, 0, 'f', 2);
                }
                if (dataArea.size() >= 8) {
                    double dval = SiemensFrameBuilder::parseDouble(frame);
                    html += QString("&nbsp;&nbsp;<b>Double:</b> %1<br>").arg(dval, 0, 'f', 3);
                }
                if (dataArea.size() >= 2) {
                    qint16 ival = SiemensFrameBuilder::parseInt16(frame);
                    html += QString("&nbsp;&nbsp;<b>Int16:</b> %1<br>").arg(ival);
                }
                if (dataArea.size() >= 4) {
                    qint32 i32val = SiemensFrameBuilder::parseInt32(frame);
                    html += QString("&nbsp;&nbsp;<b>Int32:</b> %1<br>").arg(i32val);
                }
                {
                    QString sval = SiemensFrameBuilder::parseString(frame);
                    if (!sval.isEmpty() && sval != "未加工")
                        html += QString("&nbsp;&nbsp;<b>String:</b> %1<br>").arg(sval);
                }

                // Try mode/status parse
                if (frame.size() >= 32) {
                    QString mode = SiemensFrameBuilder::parseMode(frame);
                    if (mode != "OTHER")
                        html += QString("&nbsp;&nbsp;<b>模式:</b> %1<br>").arg(mode);
                    QString status = SiemensFrameBuilder::parseStatus(frame);
                    if (status != "OTHER")
                        html += QString("&nbsp;&nbsp;<b>状态:</b> %1<br>").arg(status);
                }
            }
        }
    }

    // Full hex dump
    html += "<br><b>=== 完整帧 ===</b><br>";
    QStringList fullHex;
    for (int i = 0; i < frame.size(); ++i)
        fullHex << QString("%1").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    html += fullHex.join(" ");

    m_resultDisplay->setHtml(html);
}
