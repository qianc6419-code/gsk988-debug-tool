#include "modbusparsewidget.h"
#include "modbusprotocol.h"
#include "modbusframebuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

ModbusParseWidget::ModbusParseWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ModbusParseWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Input section
    auto* inputLabel = new QLabel("输入 HEX 帧数据（支持空格、逗号、0x 前缀）:");
    m_hexInput = new QTextEdit;
    m_hexInput->setFontFamily("Consolas");
    m_hexInput->setMaximumHeight(100);
    m_hexInput->setPlaceholderText("00 01 00 00 00 06 01 03 00 00 00 0A");

    m_parseBtn = new QPushButton("解析");
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &ModbusParseWidget::doParse);

    auto* inputBar = new QHBoxLayout;
    inputBar->addWidget(m_hexInput, 3);
    inputBar->addWidget(m_parseBtn);

    mainLayout->addWidget(inputLabel);
    mainLayout->addLayout(inputBar);

    // Result section
    auto* resultLabel = new QLabel("解析结果:");
    m_resultDisplay = new QTextEdit;
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setFontFamily("Consolas");
    m_resultDisplay->setStyleSheet("QTextEdit { font-size: 13px; }");

    mainLayout->addWidget(resultLabel);
    mainLayout->addWidget(m_resultDisplay, 1);
}

void ModbusParseWidget::doParse()
{
    QString text = m_hexInput->toPlainText();
    text.remove(' ');
    text.remove(',');
    text.remove('\n');
    text.remove('\r');
    text.remove("0x", Qt::CaseInsensitive);
    text.remove("0X");

    QByteArray frame = QByteArray::fromHex(text.toUtf8());
    if (frame.isEmpty()) {
        m_resultDisplay->setHtml("<span style='color:red;'>输入数据无效或为空</span>");
        return;
    }

    // Minimum Modbus TCP frame: MBAP(6) + UnitID(1) + FuncCode(1) = 8 bytes
    if (frame.size() < 8) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧太短：Modbus TCP 帧至少需要 8 字节（MBAP头7字节+功能码1字节）</span>");
        return;
    }

    // Verify Protocol ID
    quint16 protoId = ModbusFrameBuilder::parseProtocolId(frame);
    if (protoId != 0x0000) {
        m_resultDisplay->setHtml(QString("<span style='color:red;'>协议ID错误: 0x%1 (应为 0x0000)</span>")
                                     .arg(protoId, 4, 16, QChar('0')).toUpper());
        return;
    }

    auto coloredHex = [](const QByteArray& ba, const QString& color) -> QString {
        QStringList bytes;
        for (int i = 0; i < ba.size(); ++i)
            bytes << QString("<span style='color:%1;'>%2</span>")
                     .arg(color)
                     .arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return bytes.join(" ");
    };

    quint16 txnId = ModbusFrameBuilder::parseTransactionId(frame);
    quint16 length = ModbusFrameBuilder::parseLength(frame);
    quint8 unitId = ModbusFrameBuilder::parseUnitId(frame);
    quint8 funcCode = ModbusFrameBuilder::parseFunctionCode(frame);

    QString html;

    // MBAP Header
    html += "<b>── MBAP Header ──</b><br>";
    html += QString("  Transaction ID : <span style='color:#2196F3;'>0x%1</span> (%2)<br>")
                .arg(txnId, 4, 16, QChar('0')).toUpper().arg(txnId);
    html += QString("  Protocol ID    : <span style='color:#9E9E9E;'>0x%1</span> (Modbus)<br>")
                .arg(protoId, 4, 16, QChar('0')).toUpper();
    html += QString("  Length          : <span style='color:#FF9800;'>0x%1</span> (%2 字节)<br>")
                .arg(length, 4, 16, QChar('0')).toUpper().arg(length);
    html += QString("  Unit ID         : <span style='color:#4CAF50;'>0x%1</span> (%2)<br>")
                .arg(unitId, 2, 16, QChar('0')).toUpper().arg(unitId);

    // PDU
    html += "<b>── PDU ──</b><br>";

    bool isException = ModbusFrameBuilder::isException(funcCode);
    quint8 originalFunc = isException ? (funcCode & 0x7F) : funcCode;
    auto cmdDef = ModbusProtocol::commandDefRaw(originalFunc);

    if (isException) {
        html += QString("  Function Code : <span style='color:#F44336;'>0x%1</span> (异常! 原始: 0x%2 %3)<br>")
                    .arg(funcCode, 2, 16, QChar('0')).toUpper()
                    .arg(originalFunc, 2, 16, QChar('0')).toUpper()
                    .arg(cmdDef.name);
        if (frame.size() > 8) {
            quint8 excCode = static_cast<quint8>(frame[8]);
            QString excMsg;
            switch (excCode) {
            case 1: excMsg = "非法功能码"; break;
            case 2: excMsg = "非法数据地址"; break;
            case 3: excMsg = "非法数据值"; break;
            case 4: excMsg = "从站故障"; break;
            case 5: excMsg = "确认"; break;
            case 6: excMsg = "从站忙"; break;
            case 7: excMsg = "负确认"; break;
            case 8: excMsg = "内存奇偶错误"; break;
            default: excMsg = QString("未知(%1)").arg(excCode); break;
            }
            html += QString("  异常码: <span style='color:#F44336;'>0x%1</span> (%2)<br>")
                        .arg(excCode, 2, 16, QChar('0')).toUpper().arg(excMsg);
        }
    } else {
        html += QString("  Function Code : <span style='color:#4CAF50;'>0x%1</span> (%2)<br>")
                    .arg(funcCode, 2, 16, QChar('0')).toUpper().arg(cmdDef.name);

        QByteArray pduData = frame.mid(8);
        if (!pduData.isEmpty()) {
            html += QString("  数据: %1<br>").arg(coloredHex(pduData, "#9C27B0"));

            // Try semantic interpretation
            QString interp = ModbusProtocol::interpretDataRaw(funcCode, pduData);
            html += QString("  <b>语义:</b> %1<br>").arg(interp);
        }
    }

    // Summary
    html += "<b>── 摘要 ──</b><br>";
    if (isException) {
        html += QString("异常响应: Unit %1, 功能码 0x%2")
                    .arg(unitId).arg(originalFunc, 2, 16, QChar('0')).toUpper();
    } else {
        // Determine if this looks like a request or response
        bool looksLikeRequest = false;
        switch (funcCode) {
        case 0x01: case 0x02: case 0x03: case 0x04:
            looksLikeRequest = (frame.size() == 12); // request = MBAP(6)+UID(1)+FC(1)+addr(2)+qty(2) = 12
            break;
        case 0x05: case 0x06:
            looksLikeRequest = true; // same format for request/response
            break;
        case 0x0F: case 0x10:
            looksLikeRequest = (frame.size() > 13); // request has more data than response
            break;
        }
        if (looksLikeRequest && (funcCode == 0x01 || funcCode == 0x02 || funcCode == 0x03 || funcCode == 0x04)) {
            QByteArray pduData = frame.mid(8);
            if (pduData.size() >= 4) {
                quint16 addr = ModbusFrameBuilder::decodeUInt16(pduData, 0);
                quint16 qty = ModbusFrameBuilder::decodeUInt16(pduData, 2);
                html += QString("请求: 从 Unit %1 %2, 起始地址 %3, 数量 %4")
                            .arg(unitId).arg(cmdDef.name).arg(addr).arg(qty);
            }
        } else {
            html += QString("响应: Unit %1, 功能码 0x%2 (%3)")
                        .arg(unitId).arg(funcCode, 2, 16, QChar('0')).toUpper().arg(cmdDef.name);
        }
    }

    m_resultDisplay->setHtml(html);
}
