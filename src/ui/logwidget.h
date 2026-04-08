#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

    void appendLog(const QString &message);
    void appendLog(const QString &message, const QByteArray &data);
    void clearLog();

signals:
    void logSaved(const QString &filePath);

private slots:
    void onSaveLog();
    void onClearLog();
    void onCopyLog();

private:
    void setupUi();

    QTextEdit *m_logTextEdit;
    QPushButton *m_saveButton;
    QPushButton *m_clearButton;
    QPushButton *m_copyButton;
};

#endif // LOGWIDGET_H
