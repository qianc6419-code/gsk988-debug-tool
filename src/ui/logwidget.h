#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTextEdit>

struct LogEntry
{
    QString time;
    QString direction;
    QString cmdCode;
    QString rawHex;
    QString parsedText;
};

class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();
    void addLog(const QString &direction, const QString &cmdCode, const QByteArray &rawData, const QString &parsed = "");
    void clearLogs();
    bool saveToFile(const QString &filePath, const QString &format = "TXT");
    int logCount() const { return m_logs.size(); }
private slots:
    void onFilterChanged(int index);
    void onSaveClicked();
    void onClearClicked();
private:
    void refreshDisplay();
    QString getTimeStamp() const;
    QString bytesToHex(const QByteArray &data) const;

    QTextEdit *m_logEdit;
    QComboBox *m_filterCombo;
    QPushButton *m_saveBtn;
    QPushButton *m_clearBtn;
    QVector<LogEntry> m_logs;
    QVector<LogEntry> m_filteredLogs;
};
#endif