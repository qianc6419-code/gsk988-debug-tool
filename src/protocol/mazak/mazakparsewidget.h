#ifndef MAZAKPARSEWIDGET_H
#define MAZAKPARSEWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>

class MazakParseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MazakParseWidget(QWidget* parent = nullptr);

private slots:
    void onParse();

private:
    QComboBox* m_typeCombo;
    QTextEdit* m_inputEdit;
    QTextEdit* m_resultEdit;
};

#endif // MAZAKPARSEWIDGET_H
