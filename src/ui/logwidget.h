#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVector>
#include <QDateTime>

class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);

public slots:
    void logFrame(const QByteArray& frame, bool isSend, const QString& cmdDesc = {});
    void logError(const QString& msg);
    void clearLog();
    void exportLog();

private:
    void setupUI();
    void refreshDisplay();

    QComboBox* m_filterCombo;
    QTextEdit* m_logDisplay;
    QPushButton* m_clearBtn;
    QPushButton* m_exportBtn;

    struct LogEntry {
        QDateTime time;
        enum Type { Send, Recv, Error } type;
        QString message;
        QByteArray rawData;
    };
    QVector<LogEntry> m_entries;
};

#endif // LOGWIDGET_H
