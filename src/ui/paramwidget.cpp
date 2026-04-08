#include "paramwidget.h"

ParamWidget::ParamWidget(QWidget *parent)
    : QWidget(parent)
    , m_paramTableWidget(nullptr)
    , m_loadButton(nullptr)
    , m_saveButton(nullptr)
    , m_resetButton(nullptr)
{
    setupUi();
}

ParamWidget::~ParamWidget()
{
}

void ParamWidget::setupUi()
{
}

void ParamWidget::loadParameters()
{
}

void ParamWidget::saveParameters()
{
}

void ParamWidget::resetToDefaults()
{
}

void ParamWidget::onParameterChanged(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
}

void ParamWidget::onLoadParams()
{
}

void ParamWidget::onSaveParams()
{
}

void ParamWidget::onResetParams()
{
}
