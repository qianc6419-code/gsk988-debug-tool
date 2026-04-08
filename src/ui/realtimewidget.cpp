#include "realtimewidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QDebug>
#include <QScrollBar>

RealtimeWidget::RealtimeWidget(QWidget *parent)
    : QWidget(parent)
    , m_statusCard(nullptr)
    , m_machineCoordCard(nullptr)
    , m_workCoordCard(nullptr)
    , m_alarmCard(nullptr)
    , m_runStateLabel(nullptr)
    , m_modeLabel(nullptr)
    , m_feedLabel(nullptr)
    , m_spindleLabel(nullptr)
    , m_toolLabel(nullptr)
    , m_mxLabel(nullptr), m_myLabel(nullptr), m_mzLabel(nullptr)
    , m_wxLabel(nullptr), m_wyLabel(nullptr), m_wzLabel(nullptr)
    , m_alarmCountLabel(nullptr)
    , m_alarmDetailLabel(nullptr)
    , m_rawDataEdit(nullptr)
{
    setupUi();
}

void RealtimeWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Top 2x2 grid of cards
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(10);

    // Status card
    m_statusCard = new QGroupBox(QString::fromUtf8("\350\256\276\345\217\221\345\221\230\347\212\266\346\200\201"));
    QVBoxLayout *statusLayout = new QVBoxLayout(m_statusCard);
    m_runStateLabel = new QLabel(QString::fromUtf8("\20181"));
    m_runStateLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: green;");
    m_modeLabel = new QLabel(QString::fromUtf8("\346\250\241\345\274\217: \20181"));
    m_feedLabel = new QLabel(QString::fromUtf8("\350\277\207\347\273\203: \20181%"));
    m_spindleLabel = new QLabel(QString::fromUtf8("\344\270\273\350\275\257: \20181 RPM"));
    m_toolLabel = new QLabel(QString::fromUtf8("\345\210\266\345\217\267: T\20181"));
    statusLayout->addWidget(m_runStateLabel);
    statusLayout->addWidget(m_modeLabel);
    statusLayout->addWidget(m_feedLabel);
    statusLayout->addWidget(m_spindleLabel);
    statusLayout->addWidget(m_toolLabel);
    m_statusCard->setLayout(statusLayout);
    m_statusCard->setFixedHeight(100);

    // Machine coord card
    m_machineCoordCard = new QGroupBox(QString::fromUtf8("\346\234\272\346\240\274\345\235\200\345\277\203"));
    QVBoxLayout *machineLayout = new QVBoxLayout(m_machineCoordCard);
    m_mxLabel = new QLabel("X: \20181");
    m_myLabel = new QLabel("Y: \20181");
    m_mzLabel = new QLabel("Z: \20181");
    QFont monoFont("Consolas", 12);
    m_mxLabel->setFont(monoFont);
    m_myLabel->setFont(monoFont);
    m_mzLabel->setFont(monoFont);
    machineLayout->addWidget(m_mxLabel);
    machineLayout->addWidget(m_myLabel);
    machineLayout->addWidget(m_mzLabel);
    m_machineCoordCard->setLayout(machineLayout);
    m_machineCoordCard->setFixedHeight(100);

    // Work coord card
    m_workCoordCard = new QGroupBox(QString::fromUtf8("\345\267\245\344\273\244\345\235\200\345\277\203"));
    QVBoxLayout *workLayout = new QVBoxLayout(m_workCoordCard);
    m_wxLabel = new QLabel("X: \20181");
    m_wyLabel = new QLabel("Y: \20181");
    m_wzLabel = new QLabel("Z: \20181");
    m_wxLabel->setFont(monoFont);
    m_wyLabel->setFont(monoFont);
    m_wzLabel->setFont(monoFont);
    workLayout->addWidget(m_wxLabel);
    workLayout->addWidget(m_wyLabel);
    workLayout->addWidget(m_wzLabel);
    m_workCoordCard->setLayout(workLayout);
    m_workCoordCard->setFixedHeight(100);

    // Alarm card
    m_alarmCard = new QGroupBox(QString::fromUtf8("\346\212\245\350\256\260"));
    QVBoxLayout *alarmLayout = new QVBoxLayout(m_alarmCard);
    m_alarmCountLabel = new QLabel(QString::fromUtf8("\345\205\263 \20181 \346\235\241\346\212\245\350\256\260"));
    m_alarmDetailLabel = new QLabel(QString::fromUtf8("\346\227\240\346\212\245\350\256\260"));
    m_alarmDetailLabel->setStyleSheet("color: green;");
    alarmLayout->addWidget(m_alarmCountLabel);
    alarmLayout->addWidget(m_alarmDetailLabel);
    m_alarmCard->setLayout(alarmLayout);
    m_alarmCard->setFixedHeight(100);

    gridLayout->addWidget(m_statusCard, 0, 0);
    gridLayout->addWidget(m_machineCoordCard, 0, 1);
    gridLayout->addWidget(m_workCoordCard, 1, 0);
    gridLayout->addWidget(m_alarmCard, 1, 1);

    mainLayout->addLayout(gridLayout);

    // Raw data group at bottom
    QGroupBox *rawGroup = new QGroupBox(QString::fromUtf8("\345\216\237\345\247\220\345\220\215\346\225\260\346\215\256"));
    QVBoxLayout *rawLayout = new QVBoxLayout(rawGroup);
    m_rawDataEdit = new QTextEdit();
    m_rawDataEdit->setReadOnly(true);
    m_rawDataEdit->setFont(QFont("Consolas", 11));
    m_rawDataEdit->setStyleSheet("background-color: #1e1e2e; color: #a0e0a0;");
    m_rawDataEdit->setMaximumHeight(120);
    rawLayout->addWidget(m_rawDataEdit);
    rawGroup->setLayout(rawLayout);

    mainLayout->addWidget(rawGroup);

    setLayout(mainLayout);
}

void RealtimeWidget::updateDeviceInfo(const QMap<QString, QVariant> &data)
{
    QString modelName = data.value("modelName").toString();
    QString softwareVersion = data.value("softwareVersion").toString();
    m_runStateLabel->setText(modelName + " v" + softwareVersion);
    m_runStateLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: blue;");
}

void RealtimeWidget::updateMachineStatus(const QMap<QString, QVariant> &data)
{
    QString runStateText = data.value("runStateText").toString();
    QString runState = data.value("runState").toString();

    if (runStateText == QString::fromUtf8("\350\277\207\350\241\214\344\270\255")) {
        m_runStateLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: green;");
    } else if (runStateText == QString::fromUtf8("\345\201\234\346\255\214")) {
        m_runStateLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: red;");
    } else {
        m_runStateLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: orange;");
    }
    m_runStateLabel->setText(runStateText);

    QString mode = data.value("mode").toString();
    m_modeLabel->setText(QString::fromUtf8("\346\250\241\345\274\217: %1").arg(mode));

    int feed = data.value("feed").toInt();
    m_feedLabel->setText(QString::fromUtf8("\350\277\207\347\273\203: %1%").arg(feed));

    int spindle = data.value("spindle").toInt();
    m_spindleLabel->setText(QString::fromUtf8("\344\270\273\350\275\257: %1 RPM").arg(spindle));

    int tool = data.value("tool").toInt();
    m_toolLabel->setText(QString::fromUtf8("\345\210\266\345\217\267: T%1").arg(tool));
}

void RealtimeWidget::updateCoordinates(const QMap<QString, QVariant> &data)
{
    double mx = data.value("mx").toDouble();
    double my = data.value("my").toDouble();
    double mz = data.value("mz").toDouble();
    m_mxLabel->setText(QString("X: %1").arg(mx, 0, 'f', 3));
    m_myLabel->setText(QString("Y: %1").arg(my, 0, 'f', 3));
    m_mzLabel->setText(QString("Z: %1").arg(mz, 0, 'f', 3));

    double wx = data.value("wx").toDouble();
    double wy = data.value("wy").toDouble();
    double wz = data.value("wz").toDouble();
    m_wxLabel->setText(QString("X: %1").arg(wx, 0, 'f', 3));
    m_wyLabel->setText(QString("Y: %1").arg(wy, 0, 'f', 3));
    m_wzLabel->setText(QString("Z: %1").arg(wz, 0, 'f', 3));
}

void RealtimeWidget::updateAlarms(const QMap<QString, QVariant> &data)
{
    int count = data.value("count").toInt();
    m_alarmCountLabel->setText(QString::fromUtf8("\345\205\263 %1 \346\235\241\346\212\245\350\256\260").arg(count));

    if (count == 0) {
        m_alarmDetailLabel->setText(QString::fromUtf8("\346\227\240\346\212\245\350\256\260"));
        m_alarmDetailLabel->setStyleSheet("color: green;");
    } else {
        QStringList alarms;
        for (int i = 0; i < count; ++i) {
            QString alarmText = data.value(QString("alarm%1").arg(i)).toString();
            if (!alarmText.isEmpty()) {
                alarms.append(alarmText);
            }
        }
        m_alarmDetailLabel->setText(alarms.join("\n"));
        m_alarmDetailLabel->setStyleSheet("color: red;");
    }
}

void RealtimeWidget::appendRawData(const QByteArray &hex)
{
    QString display = hexToDisplay(hex);
    m_rawDataEdit->append(display);

    // Limit to 100 lines
    QTextCursor cursor = m_rawDataEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    int lineCount = 0;
    while (!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
        lineCount++;
    }
    while (lineCount > 100) {
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar();
        lineCount--;
    }
}

QString RealtimeWidget::hexToDisplay(const QByteArray &data)
{
    QString result;
    for (int i = 0; i < data.size(); ++i) {
        result += QString::number((unsigned char)data[i], 16).rightJustified(2, '0').toUpper();
        if ((i + 1) % 16 == 0) {
            result += "\n";
        } else if ((i + 1) % 8 == 0) {
            result += "  ";
        } else {
            result += " ";
        }
    }
    return result;
}

void RealtimeWidget::clear()
{
    m_runStateLabel->setText(QString::fromUtf8("\20181"));
    m_modeLabel->setText(QString::fromUtf8("\346\250\241\345\274\217: \20181"));
    m_feedLabel->setText(QString::fromUtf8("\350\277\207\347\273\203: \20181%"));
    m_spindleLabel->setText(QString::fromUtf8("\344\270\273\350\275\257: \20181 RPM"));
    m_toolLabel->setText(QString::fromUtf8("\345\210\266\345\217\267: T\20181"));
    m_mxLabel->setText("X: \20181");
    m_myLabel->setText("Y: \20181");
    m_mzLabel->setText("Z: \20181");
    m_wxLabel->setText("X: \20181");
    m_wyLabel->setText("Y: \20181");
    m_wzLabel->setText("Z: \20181");
    m_alarmCountLabel->setText(QString::fromUtf8("\345\205\263 \20181 \346\235\241\346\212\245\350\256\260"));
    m_alarmDetailLabel->setText(QString::fromUtf8("\346\227\240\346\212\245\350\256\260"));
    m_rawDataEdit->clear();
}
