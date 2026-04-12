#include "gsk988parsewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

Gsk988ParseWidget::Gsk988ParseWidget(QWidget* parent)
    : QWidget(parent)
    , m_protocol(new Gsk988Protocol(this))
{
    setupUI();
}

void Gsk988ParseWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Input section
    auto* inputLabel = new QLabel("输入 HEX 帧数据（支持空格、逗号、0x 前缀）:");
    m_hexInput = new QTextEdit;
    m_hexInput->setFontFamily("Consolas");
    m_hexInput->setMaximumHeight(100);
    m_hexInput->setPlaceholderText("93 00 00 09 6F C8 1E 64 1E 17 10 17 21 00 55 AA");

    m_parseBtn = new QPushButton("解析");
    m_parseBtn->setFixedWidth(80);
    connect(m_parseBtn, &QPushButton::clicked, this, &Gsk988ParseWidget::doParse);

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

void Gsk988ParseWidget::doParse()
{
    QString text = m_hexInput->toPlainText();
    // Clean input: remove spaces, commas, 0x prefixes
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

    if (!Gsk988FrameBuilder::validateFrame(frame)) {
        m_resultDisplay->setHtml("<span style='color:red;'>帧校验失败：请检查帧头(93 00)、帧尾(55 AA)和长度是否正确</span>");
        return;
    }

    // Determine frame type by checking identifier
    int idOffset = 4; // after head(2) + length(2)
    QByteArray identifier = frame.mid(idOffset, 8);
    bool isRequest = (identifier == Gsk988FrameBuilder::REQUEST_ID);
    bool isResponse = (identifier == Gsk988FrameBuilder::RESPONSE_ID);

    // Color mapping
    auto coloredHex = [](const QByteArray& ba, const QString& color) -> QString {
        QStringList bytes;
        for (int i = 0; i < ba.size(); ++i)
            bytes << QString("<span style='color:%1;'>%2</span>")
                     .arg(color)
                     .arg(static_cast<quint8>(ba[i]), 2, 16, QChar('0')).toUpper();
        return bytes.join(" ");
    };

    // Build colored display
    QString html;

    // Head
    html += "<b>帧头:</b> " + coloredHex(frame.mid(0, 2), "#2196F3") + " &nbsp; ";

    // Length
    quint16 len = (static_cast<quint8>(frame[2]) << 8) | static_cast<quint8>(frame[3]);
    html += QString("<b>长度:</b> %1 (%2字节) ").arg(coloredHex(frame.mid(2, 2), "#9E9E9E")).arg(len);

    // Identifier
    html += "<br><b>标识:</b> " + coloredHex(frame.mid(4, 8), "#FF9800");
    html += QString(" (%1)").arg(isRequest ? "请求帧" : isResponse ? "响应帧" : "未知");

    // Data field
    QByteArray dataField = frame.mid(12, len - 8);
    if (!dataField.isEmpty()) {
        quint8 cmdCode = static_cast<quint8>(dataField[0]);
        html += QString("<br><b>命令码:</b> <span style='color:#4CAF50;'>%1</span> (%2)")
                    .arg(cmdCode, 2, 16, QChar('0')).toUpper()
                    .arg(Gsk988Protocol::commandDefRaw(cmdCode).name);

        if (isResponse && dataField.size() >= 3) {
            QByteArray additionalData = dataField.mid(3);
            if (!additionalData.isEmpty()) {
                html += "<br><b>附加数据:</b> " + coloredHex(additionalData, "#9C27B0");

                // Semantic interpretation for response frames
                QString interp = Gsk988Protocol::interpretDataRaw(cmdCode, additionalData);
                html += QString("<br><b>语义解析:</b> %1").arg(interp);
            }
        } else if (isRequest) {
            QByteArray params = dataField.mid(1);
            if (!params.isEmpty()) {
                html += "<br><b>请求参数:</b> " + coloredHex(params, "#9C27B0");
            }
        }
    }

    // Tail
    html += "<br><b>帧尾:</b> " + coloredHex(frame.mid(frame.size() - 2, 2), "#2196F3");

    m_resultDisplay->setHtml(html);
}
