#include "logwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QTime>

LogWidget::LogWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LogWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Top toolbar
    auto* toolbar = new QHBoxLayout;
    m_filterCombo = new QComboBox;
    m_filterCombo->addItems({"全部", "发送", "接收", "错误"});
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogWidget::refreshDisplay);

    m_clearBtn = new QPushButton("清空");
    connect(m_clearBtn, &QPushButton::clicked, this, &LogWidget::clearLog);

    m_exportBtn = new QPushButton("导出");
    connect(m_exportBtn, &QPushButton::clicked, this, &LogWidget::exportLog);

    toolbar->addWidget(m_filterCombo);
    toolbar->addStretch();
    toolbar->addWidget(m_clearBtn);
    toolbar->addWidget(m_exportBtn);

    // Log display
    m_logDisplay = new QTextEdit;
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setFontFamily("Consolas");
    m_logDisplay->setStyleSheet("QTextEdit { font-size: 12px; }");

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_logDisplay);
}

void LogWidget::logFrame(const QByteArray& frame, bool isSend, const QString& cmdDesc)
{
    LogEntry entry;
    entry.time = QDateTime::currentDateTime();
    entry.type = isSend ? LogEntry::Send : LogEntry::Recv;
    entry.rawData = frame;

    QString dir = isSend ? "发送" : "接收";
    QString timeStr = entry.time.toString("HH:mm:ss.zzz");

    // Format hex
    QStringList hexBytes;
    for (int i = 0; i < frame.size(); ++i) {
        hexBytes << QString("%1").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }

    entry.message = QString("[%1] [%2] %3\n%4")
                        .arg(timeStr, dir, cmdDesc, hexBytes.join(" "));

    m_entries.append(entry);
    refreshDisplay();
}

void LogWidget::logError(const QString& msg)
{
    LogEntry entry;
    entry.time = QDateTime::currentDateTime();
    entry.type = LogEntry::Error;
    entry.message = QString("[%1] [错误] %2").arg(entry.time.toString("HH:mm:ss.zzz"), msg);

    m_entries.append(entry);
    refreshDisplay();
}

void LogWidget::clearLog()
{
    m_entries.clear();
    m_logDisplay->clear();
}

void LogWidget::exportLog()
{
    QString path = QFileDialog::getSaveFileName(this, "导出日志", "comm_log.txt", "文本文件 (*.txt)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");
        for (const auto& e : m_entries) {
            out << e.message << "\n\n";
        }
    }
}

void LogWidget::refreshDisplay()
{
    int filterIdx = m_filterCombo->currentIndex();

    QStringList lines;
    for (const auto& e : m_entries) {
        bool show = false;
        switch (filterIdx) {
        case 0: show = true; break;
        case 1: show = (e.type == LogEntry::Send); break;
        case 2: show = (e.type == LogEntry::Recv); break;
        case 3: show = (e.type == LogEntry::Error); break;
        }
        if (show) lines << e.message;
    }

    m_logDisplay->setPlainText(lines.join("\n\n"));
    // Scroll to bottom
    QTextCursor c = m_logDisplay->textCursor();
    c.movePosition(QTextCursor::End);
    m_logDisplay->setTextCursor(c);
}
