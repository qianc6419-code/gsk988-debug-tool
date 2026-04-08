#ifndef DATAPARSEWIDGET_H
#define DATAPARSEWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTextEdit>

class DataParseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataParseWidget(QWidget *parent = nullptr);
    ~DataParseWidget();

    void setRawData(const QByteArray &data);
    void clearParsedData();

signals:
    void dataParsed(const QString &parsedText);

private slots:
    void onItemExpanded(QTreeWidgetItem *item);
    void onExportData();

private:
    void setupUi();
    void parseData(const QByteArray &data);

    QTreeWidget *m_treeWidget;
    QTextEdit *m_rawDataEdit;
    QTextEdit *m_parsedTextEdit;
};

#endif // DATAPARSEWIDGET_H
