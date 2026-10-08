#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QStringList>
#include <QDate>
//these includes help with styles
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// Global or tracked state for current view
QDate currentDisplayDate = QDate::currentDate();

// Clear existing buttons in layout and redraw grid for current month/year
void updateCalendarGrid(QGridLayout *grid, QLabel *monthLabel, QLabel *selectedLabel) {
    // 1. Update Month Header Text
    monthLabel->setText(currentDisplayDate.toString("MMMM yyyy"));

    // 2. Clear old day buttons from grid
    QLayoutItem *child;
    while ((child = grid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    // 3. Find starting day of week for current month (1 = Mon, 7 = Sun)
    QDate firstOfMonth(currentDisplayDate.year(), currentDisplayDate.month(), 1);
    int startCol = firstOfMonth.dayOfWeek() - 1; // 0-indexed column offset
    int daysInMonth = currentDisplayDate.daysInMonth();

    int dayNumber = 1;
    int row = 0;

    for (int col = startCol; dayNumber <= daysInMonth; ++col) {
        if (col == 7) {
            col = 0;
            row++;
        }

        QPushButton *dayBtn = new QPushButton(QString::number(dayNumber));
        dayBtn->setMinimumSize(40, 40);

        int clickedDay = dayNumber;
        QObject::connect(dayBtn, &QPushButton::clicked, [clickedDay, selectedLabel]() {
            QDate clickedDate(currentDisplayDate.year(), currentDisplayDate.month(), clickedDay);
            selectedLabel->setText("Selected Date: " + clickedDate.toString("yyyy-MM-dd"));
        });

        grid->addWidget(dayBtn, row, col);
        dayNumber++;
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

// Build absolute path to styles.qss in the root or build folder
QString stylePath = QCoreApplication::applicationDirPath() + "/../styles.qss";
    QFile file(stylePath);
    bool styleOpened = file.open(QFile::ReadOnly | QFile::Text);

    if (!styleOpened) {
        file.setFileName("styles.qss");
        styleOpened = file.open(QFile::ReadOnly | QFile::Text);
        if (!styleOpened) {
            qWarning() << "Could not open styles.qss!";
        }
    }

    if (file.isOpen()) {
        QTextStream stream(&file);
        app.setStyleSheet(stream.readAll());
        file.close();
    }

    QWidget window;
    window.setWindowTitle("Custom Grid Calendar");
    window.resize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(&window);

    // --- Header Navigation ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QPushButton *prevBtn = new QPushButton("<");
    QLabel *monthLabel = new QLabel();
    QPushButton *nextBtn = new QPushButton(">");

    headerLayout->addWidget(prevBtn);
    headerLayout->addStretch();
    headerLayout->addWidget(monthLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(nextBtn);
    mainLayout->addLayout(headerLayout);

    // --- Weekday Headers ---
    QGridLayout *daysHeaderLayout = new QGridLayout();
    QStringList days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (int col = 0; col < 7; ++col) {
        QLabel *dayLabel = new QLabel(days[col]);
        dayLabel->setAlignment(Qt::AlignCenter);
        daysHeaderLayout->addWidget(dayLabel, 0, col);
    }
    mainLayout->addLayout(daysHeaderLayout);

    // --- Dynamic Month Grid & Selected Label ---
    QGridLayout *calendarGrid = new QGridLayout();
    mainLayout->addLayout(calendarGrid);

    QLabel *selectedDateLabel = new QLabel("Selected Date: None");
    selectedDateLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(selectedDateLabel);

    // --- Button Click Signal Connections ---
    QObject::connect(nextBtn, &QPushButton::clicked, [&]() {
        currentDisplayDate = currentDisplayDate.addMonths(1);
        updateCalendarGrid(calendarGrid, monthLabel, selectedDateLabel);
    });

    QObject::connect(prevBtn, &QPushButton::clicked, [&]() {
        currentDisplayDate = currentDisplayDate.addMonths(-1);
        updateCalendarGrid(calendarGrid, monthLabel, selectedDateLabel);
    });

    // Initial grid population
    updateCalendarGrid(calendarGrid, monthLabel, selectedDateLabel);

    window.show();
    return app.exec();
}