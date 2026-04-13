#ifndef KNDPARSEWIDGET_H
#define KNDPARSEWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>

class KndParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KndParseWidget(QWidget* parent = nullptr);

private slots:
    void onParse();

private:
    QComboBox* m_typeCombo;
    QTextEdit* m_inputEdit;
    QTextEdit* m_resultEdit;
};

#endif // KNDPARSEWIDGET_H