#include "dataparsewidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QFont>
#include <QPalette>

DataParseWidget::DataParseWidget(QWidget *parent)
    : QWidget(parent)
    , m_inputEdit(nullptr)
    , m_outputEdit(nullptr)
    , m_parseBtn(nullptr)
{
    setupUi();
}

void DataParseWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Input group
    QGroupBox *inputGroup = new QGroupBox("输入数据", this);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputGroup);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("粘贴协议帧 HEX 数据，如: 5A A5 00 7E 01 00 00 00 00...");
    QFont inputFont("Consolas", 11);
    m_inputEdit->setFont(inputFont);
    QPalette inputPalette = m_inputEdit->palette();
    inputPalette.setColor(QPalette::Base, QColor(0xf5, 0xf5, 0xf5));
    m_inputEdit->setPalette(inputPalette);

    inputLayout->addWidget(m_inputEdit);

    // Parse button
    m_parseBtn = new QPushButton("解析", this);
    m_parseBtn->setStyleSheet("QPushButton { background-color: #3498DB; color: white; border: none; padding: 8px 16px; }");
    mainLayout->addWidget(m_parseBtn);

    // Output group
    QGroupBox *outputGroup = new QGroupBox("解析结果", this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);

    m_outputEdit = new QTextEdit(this);
    m_outputEdit->setReadOnly(true);
    QFont outputFont("Consolas", 11);
    m_outputEdit->setFont(outputFont);
    QPalette outputPalette = m_outputEdit->palette();
    outputPalette.setColor(QPalette::Base, QColor(0xfa, 0xfa, 0xfa));
    m_outputEdit->setPalette(outputPalette);

    outputLayout->addWidget(m_outputEdit);

    mainLayout->addWidget(inputGroup);
    mainLayout->addWidget(outputGroup);

    setLayout(mainLayout);

    connect(m_parseBtn, &QPushButton::clicked, this, &DataParseWidget::onParseClicked);
}

void DataParseWidget::onParseClicked()
{
    QString hexInput = m_inputEdit->toPlainText();
    QString result = parseHexToStructure(hexInput);
    m_outputEdit->setPlainText(result);
}

QString DataParseWidget::parseHexToStructure(const QString &hexInput)
{
    // Split by whitespace and filter empty strings
    QStringList hexList = hexInput.split(QRegExp("\\s+"), Qt::SkipEmptyParts);

    if (hexList.isEmpty()) {
        return "错误: 输入为空";
    }

    // Convert hex strings to bytes
    QByteArray bytes;
    for (const QString &hex : hexList) {
        bool ok;
        int value = hex.toInt(&ok, 16);
        if (!ok) {
            return QString("错误: 无效的HEX数据: %1").arg(hex);
        }
        bytes.append(static_cast<char>(value));
    }

    if (bytes.size() < 9) {
        return QString("错误: 数据长度不足 (实际: %1, 要求: 9)").arg(bytes.size());
    }

    // Parse structure
    quint8 frameHead0 = static_cast<quint8>(bytes[0]);
    quint8 frameHead1 = static_cast<quint8>(bytes[1]);
    quint16 length = (static_cast<quint8>(bytes[2]) << 8) | static_cast<quint8>(bytes[3]);
    quint8 cmd = static_cast<quint8>(bytes[4]);
    quint8 sub = static_cast<quint8>(bytes[5]);
    quint8 type = static_cast<quint8>(bytes[6]);

    // Data (bytes 7 to size-3, excluding last 2 bytes for XOR and CRC)
    int dataEnd = bytes.size() - 3;
    QString dataStr;
    for (int i = 7; i <= dataEnd && i < bytes.size(); ++i) {
        dataStr += QString("%1 ").arg(static_cast<quint8>(bytes[i]), 2, 16, QChar('0')).toUpper();
    }
    dataStr = dataStr.trimmed();

    // XOR (second to last byte)
    quint8 xorVal = static_cast<quint8>(bytes[bytes.size() - 2]);

    // CRC16 (last byte, actually 2 bytes in big endian)
    quint16 crc16 = (static_cast<quint8>(bytes[bytes.size() - 1]) << 8) | static_cast<quint8>(bytes[bytes.size()]);

    // Build output
    QString output;
    output += QString("【帧头】 %1 %2\n")
                  .arg(QString("%1").arg(frameHead0, 2, 16, QChar('0')).toUpper())
                  .arg(QString("%1").arg(frameHead1, 2, 16, QChar('0')).toUpper());
    output += QString("【长度】 %1 (0x%2)\n")
                  .arg(length)
                  .arg(QString::number(length, 16).toUpper());
    output += QString("【命令】 CMD=0x%1 SUB=0x%2 Type=0x%3\n")
                  .arg(QString::number(cmd, 16).toUpper())
                  .arg(QString::number(sub, 16).toUpper())
                  .arg(QString::number(type, 16).toUpper());
    output += QString("【数据】 %1 (%2 bytes)\n")
                  .arg(dataStr)
                  .arg(dataEnd >= 7 ? dataEnd - 7 + 1 : 0);
    output += QString("【XOR】 0x%1\n")
                  .arg(QString::number(xorVal, 16).toUpper());
    output += QString("【CRC16】 0x%1")
                  .arg(QString::number(crc16, 16).toUpper());

    return output;
}