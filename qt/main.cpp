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
#include <QDialog>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include "../src/calendar.h"
#include <chrono>

// Global or tracked state for current view
QDate currentDisplayDate = QDate::currentDate();

// Clear existing buttons in layout and redraw grid for current month/year
void updateCalendarGrid(QGridLayout *grid, QLabel *monthLabel, QLabel *selectedLabel,
                        QWidget *window, Calendar *calendar) {
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
        QObject::connect(dayBtn, &QPushButton::clicked, [clickedDay, selectedLabel, window, calendar]() {
        QDate clickedDate(currentDisplayDate.year(), currentDisplayDate.month(), clickedDay);
        selectedLabel->setText("Selected Date: " + clickedDate.toString("yyyy-MM-dd"));

        // 1. Open Dialog to get event details from the user
        QDialog dialog(window);
        dialog.setWindowTitle("Add Event");
        dialog.resize(300, 200);

        QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
        
        QLabel *dateHeader = new QLabel("Add event for: " + clickedDate.toString("yyyy-MM-dd"));
        
        // Input fields for your event details
        QLineEdit *idInput = new QLineEdit(&dialog);
        idInput->setPlaceholderText("Enter Event ID (integer)...");
        
        QLineEdit *titleInput = new QLineEdit(&dialog);
        titleInput->setPlaceholderText("Enter Event Title...");

        QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);

        dialogLayout->addWidget(dateHeader);
        dialogLayout->addWidget(idInput);
        dialogLayout->addWidget(titleInput);
        dialogLayout->addWidget(buttonBox);

        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        // 2. When user clicks Save/Accept, construct the event and call your backend function
        if (dialog.exec() == QDialog::Accepted) {
            bool ok;
            int eventId = idInput->text().toInt(&ok);
            QString eventTitle = titleInput->text();

            if (ok) {
                const auto eventDate = std::chrono::year_month_day{
                    std::chrono::year{clickedDate.year()},
                    std::chrono::month{static_cast<unsigned>(clickedDate.month())},
                    std::chrono::day{static_cast<unsigned>(clickedDate.day())}
                };
                const auto eventTime = std::chrono::system_clock::time_point{
                    std::chrono::duration_cast<std::chrono::system_clock::duration>(
                        std::chrono::sys_days{eventDate}.time_since_epoch())
                };
                
                bool success = calendar->addEvent(
                    eventTitle.toStdString(),
                    "Default Location",
                    eventTime,
                    std::chrono::minutes(30), // Default get ready offset
                    std::chrono::minutes(15)  // Default leave offset
                );

                if (success) {
                    qDebug() << "Event successfully added to backend!";
                } else {
                    qDebug() << "Failed to add event: Duplicate ID detected.";
                }
            } else {
                qDebug() << "Invalid Event ID entered!";
            }
        }
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
    Calendar calendar;
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
        updateCalendarGrid(calendarGrid, monthLabel, selectedDateLabel, &window, &calendar);
    });

    QObject::connect(prevBtn, &QPushButton::clicked, [&]() {
        currentDisplayDate = currentDisplayDate.addMonths(-1);
        updateCalendarGrid(calendarGrid, monthLabel, selectedDateLabel, &window, &calendar);
    });

    // Initial grid population
    updateCalendarGrid(calendarGrid, monthLabel, selectedDateLabel, &window, &calendar);

    window.show();
    return app.exec();
}