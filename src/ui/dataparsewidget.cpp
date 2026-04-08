#include "dataparsewidget.h"

DataParseWidget::DataParseWidget(QWidget *parent)
    : QWidget(parent)
    , m_treeWidget(nullptr)
    , m_rawDataEdit(nullptr)
    , m_parsedTextEdit(nullptr)
{
    setupUi();
}

DataParseWidget::~DataParseWidget()
{
}

void DataParseWidget::setupUi()
{
}

void DataParseWidget::setRawData(const QByteArray &data)
{
    Q_UNUSED(data);
}

void DataParseWidget::clearParsedData()
{
}

void DataParseWidget::parseData(const QByteArray &data)
{
    Q_UNUSED(data);
}

void DataParseWidget::onItemExpanded(QTreeWidgetItem *item)
{
    Q_UNUSED(item);
}

void DataParseWidget::onExportData()
{
}
