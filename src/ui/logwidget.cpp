#include "logwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFont>
#include <QColor>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , m_logEdit(nullptr)
    , m_filterCombo(nullptr)
    , m_saveBtn(nullptr)
    , m_clearBtn(nullptr)
{
    // Toolbar layout
    QHBoxLayout *toolbar = new QHBoxLayout();

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem("全部");
    m_filterCombo->addItem("发送");
    m_filterCombo->addItem("接收");
    m_filterCombo->addItem("错误");
    toolbar->addWidget(m_filterCombo);

    m_saveBtn = new QPushButton("保存", this);
    m_saveBtn->setObjectName("saveBtn");
    toolbar->addWidget(m_saveBtn);

    m_clearBtn = new QPushButton("清空", this);
    m_clearBtn->setObjectName("clearBtn");
    m_clearBtn->setStyleSheet("QPushButton { color: #f87171; }");
    toolbar->addWidget(m_clearBtn);

    toolbar->addStretch();

    // Log display
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setStyleSheet(
        "QTextEdit { background-color: #1e1e2e; color: #d0d0d0; font-family: Consolas; font-size: 11px; }"
    );

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_logEdit);

    setLayout(mainLayout);

    // Connections
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::onFilterChanged);
    connect(m_saveBtn, &QPushButton::clicked, this, &LogWidget::onSaveClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &LogWidget::onClearClicked);
}

LogWidget::~LogWidget()
{
}

void LogWidget::addLog(const QString &direction, const QString &cmdCode, const QByteArray &rawData, const QString &parsed)
{
    LogEntry entry;
    entry.time = getTimeStamp();
    entry.direction = direction;
    entry.cmdCode = cmdCode;
    entry.rawHex = bytesToHex(rawData);
    entry.parsedText = parsed;

    m_logs.append(entry);
    refreshDisplay();
}

void LogWidget::clearLogs()
{
    m_logs.clear();
    m_filteredLogs.clear();
    m_logEdit->clear();
}

bool LogWidget::saveToFile(const QString &filePath, const QString &format)
{
    Q_UNUSED(format);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    QDateTime now = QDateTime::currentDateTime();
    out << "===== Communication Log =====\n";
    out << "Time: " << now.toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << "Total Entries: " << m_logs.size() << "\n";
    out << "============================\n\n";

    for (const LogEntry &entry : m_logs) {
        out << "[" << entry.time << "] [" << entry.direction << "] [" << entry.cmdCode << "]\n";
        out << "HEX: " << entry.rawHex << "\n";
        if (!entry.parsedText.isEmpty()) {
            out << entry.parsedText << "\n";
        }
        out << "\n";
    }

    file.close();
    return true;
}

void LogWidget::onFilterChanged(int index)
{
    Q_UNUSED(index);
    refreshDisplay();
}

void LogWidget::onSaveClicked()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Log"),
        QString(),
        tr("Text Files (*.txt);;All Files (*)")
    );

    if (!filePath.isEmpty()) {
        saveToFile(filePath, "TXT");
    }
}

void LogWidget::onClearClicked()
{
    clearLogs();
}

void LogWidget::refreshDisplay()
{
    m_filteredLogs.clear();

    QString filterText = m_filterCombo->currentText();

    for (const LogEntry &entry : m_logs) {
        if (filterText == "全部" ||
            (filterText == "发送" && entry.direction == "发送") ||
            (filterText == "接收" && entry.direction == "接收") ||
            (filterText == "错误" && entry.direction == "错误")) {
            m_filteredLogs.append(entry);
        }
    }

    m_logEdit->clear();

    for (const LogEntry &entry : m_filteredLogs) {
        QString directionColor;
        if (entry.direction == "发送") {
            directionColor = "#60a5fa";
        } else if (entry.direction == "接收") {
            directionColor = "#4ade80";
        } else if (entry.direction == "错误") {
            directionColor = "#f87171";
        } else {
            directionColor = "#d0d0d0";
        }

        QString html = QString(
            "<span style=\"color: gray;\">[%1]</span> "
            "<span style=\"color: %2;\">[%3]</span> "
            "<span style=\"color: #fbbf24;\">[%4]</span><br>"
            "<span style=\"color: #a0e0a0;\">HEX: %5</span><br>"
        ).arg(entry.time, directionColor, entry.direction, entry.cmdCode, entry.rawHex);

        if (!entry.parsedText.isEmpty()) {
            html += QString("<span style=\"color: #d0d0d0;\">%1</span><br>").arg(entry.parsedText);
        }

        html += "<br>";
        m_logEdit->append(html);
    }
}

QString LogWidget::getTimeStamp() const
{
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
}

QString LogWidget::bytesToHex(const QByteArray &data) const
{
    QString hex;
    for (int i = 0; i < data.size(); ++i) {
        if (i > 0) hex += " ";
        hex += QString("%1").arg(static_cast<unsigned char>(data[i]), 2, 16, QChar('0')).toUpper();
    }
    return hex;
}