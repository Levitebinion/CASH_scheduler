#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QStringList>
#include <QDate>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QFormLayout>
#include <QTimer>

#include "../src/calendar.h"
#include <chrono>

QDate currentDisplayDate = QDate::currentDate();

QDateTime toQtDateTime(std::chrono::system_clock::time_point time) {
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            time.time_since_epoch()
        ).count();

    return QDateTime::fromMSecsSinceEpoch(milliseconds);
}

std::chrono::system_clock::time_point toChronoTime(
    const QDateTime &dateTime
) {
    return std::chrono::system_clock::time_point{
        std::chrono::milliseconds{
            dateTime.toMSecsSinceEpoch()
        }
    };
}

void updateEventCounter(
    QLabel *eventCounter,
    Calendar *calendar
) {
    eventCounter->setText(
        "Total Events: " +
        QString::number(calendar->getEventCount())
    );
}

void updateCalendarGrid(
    QGridLayout *grid,
    QLabel *monthLabel,
    QLabel *selectedLabel,
    QWidget *window,
    Calendar *calendar,
    QLabel *eventCounter
) {
    monthLabel->setText(
        currentDisplayDate.toString("MMMM yyyy")
    );

    QLayoutItem *child;

    while ((child = grid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }

        delete child;
    }

    QDate firstOfMonth(
        currentDisplayDate.year(),
        currentDisplayDate.month(),
        1
    );

    int startCol = firstOfMonth.dayOfWeek() - 1;
    int daysInMonth = currentDisplayDate.daysInMonth();

    int dayNumber = 1;
    int row = 0;

    for (int col = startCol; dayNumber <= daysInMonth; ++col) {
        if (col == 7) {
            col = 0;
            row++;
        }

        QPushButton *dayBtn =
            new QPushButton(QString::number(dayNumber));

        dayBtn->setMinimumSize(40, 40);

        int clickedDay = dayNumber;

        QObject::connect(
            dayBtn,
            &QPushButton::clicked,
            [clickedDay, selectedLabel, window, calendar, eventCounter]() {

                QDate clickedDate(
                    currentDisplayDate.year(),
                    currentDisplayDate.month(),
                    clickedDay
                );

                selectedLabel->setText(
                    "Selected Date: " +
                    clickedDate.toString("yyyy-MM-dd")
                );

                QDialog dialog(window);

                dialog.setWindowTitle("Add Event");
                dialog.resize(350, 250);

                QVBoxLayout *dialogLayout =
                    new QVBoxLayout(&dialog);

                QLabel *dateHeader = new QLabel(
                    "Add event for: " +
                    clickedDate.toString("yyyy-MM-dd")
                );

                QLineEdit *idInput =
                    new QLineEdit(&dialog);

                idInput->setPlaceholderText(
                    "Enter Event ID..."
                );

                QLineEdit *titleInput =
                    new QLineEdit(&dialog);

                titleInput->setPlaceholderText(
                    "Enter Event Title..."
                );

                QDialogButtonBox *buttonBox =
                    new QDialogButtonBox(
                        QDialogButtonBox::Save |
                        QDialogButtonBox::Cancel,
                        &dialog
                    );

                dialogLayout->addWidget(dateHeader);
                dialogLayout->addWidget(idInput);
                dialogLayout->addWidget(titleInput);
                dialogLayout->addWidget(buttonBox);

                QObject::connect(
                    buttonBox,
                    &QDialogButtonBox::accepted,
                    &dialog,
                    &QDialog::accept
                );

                QObject::connect(
                    buttonBox,
                    &QDialogButtonBox::rejected,
                    &dialog,
                    &QDialog::reject
                );

                if (dialog.exec() == QDialog::Accepted) {
                    bool ok;

                    int eventId =
                        idInput->text().toInt(&ok);

                    QString eventTitle =
                        titleInput->text();

                    if (!ok) {
                        QMessageBox::warning(
                            window,
                            "Invalid Event ID",
                            "Please enter a valid integer Event ID."
                        );
                        return;
                    }

                    if (eventTitle.trimmed().isEmpty()) {
                        QMessageBox::warning(
                            window,
                            "Invalid Event Title",
                            "Please enter an event title."
                        );
                        return;
                    }

                    QDateTime selectedDateTime(
                        clickedDate,
                        QTime(0, 0)
                    );

                    auto eventTime =
                        toChronoTime(selectedDateTime);

                    CalendarEvent newEvent(
                        eventId,
                        eventTitle.toStdString(),
                        "",
                        eventTime,
                        std::chrono::minutes{0},
                        std::chrono::minutes{0}
                    );

                    bool success =
                        calendar->addEvent(newEvent);

                    if (success) {
                        QMessageBox::information(
                            window,
                            "Event Added",
                            "Event successfully added!"
                        );

                        updateEventCounter(
                            eventCounter,
                            calendar
                        );

                        qDebug()
                            << "Event successfully added!";
                    } else {
                        QMessageBox::warning(
                            window,
                            "Duplicate Event ID",
                            "An event with that ID already exists."
                        );
                    }
                }
            }
        );

        grid->addWidget(dayBtn, row, col);
        dayNumber++;
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QString stylePath =
        QCoreApplication::applicationDirPath()
        + "/../styles.qss";

    QFile file(stylePath);

    bool styleOpened =
        file.open(QFile::ReadOnly | QFile::Text);

    if (!styleOpened) {
        file.setFileName("styles.qss");

        styleOpened =
            file.open(QFile::ReadOnly | QFile::Text);

        if (!styleOpened) {
            qWarning()
                << "Could not open styles.qss!";
        }
    }

    if (file.isOpen()) {
        QTextStream stream(&file);
        app.setStyleSheet(stream.readAll());
        file.close();
    }

    QWidget window;
    Calendar calendar;

    window.setWindowTitle("CASH Scheduler");
    window.resize(650, 600);

    QVBoxLayout *mainLayout =
        new QVBoxLayout(&window);

    QHBoxLayout *headerLayout =
        new QHBoxLayout();

    QPushButton *prevBtn =
        new QPushButton("<");

    QLabel *monthLabel =
        new QLabel();

    QPushButton *nextBtn =
        new QPushButton(">");

    headerLayout->addWidget(prevBtn);
    headerLayout->addStretch();
    headerLayout->addWidget(monthLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(nextBtn);

    mainLayout->addLayout(headerLayout);

    QGridLayout *daysHeaderLayout =
        new QGridLayout();

    QStringList days = {
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat",
        "Sun"
    };

    for (int col = 0; col < 7; ++col) {
        QLabel *dayLabel =
            new QLabel(days[col]);

        dayLabel->setAlignment(Qt::AlignCenter);

        daysHeaderLayout->addWidget(
            dayLabel,
            0,
            col
        );
    }

    mainLayout->addLayout(daysHeaderLayout);

    QGridLayout *calendarGrid =
        new QGridLayout();

    mainLayout->addLayout(calendarGrid);

    QLabel *selectedDateLabel =
        new QLabel("Selected Date: None");

    selectedDateLabel->setAlignment(
        Qt::AlignCenter
    );

    mainLayout->addWidget(selectedDateLabel);

    QLabel *eventCounter =
        new QLabel("Total Events: 0");

    eventCounter->setAlignment(
        Qt::AlignCenter
    );

    mainLayout->addWidget(eventCounter);

    QHBoxLayout *buttonLayout =
        new QHBoxLayout();

    QPushButton *searchBtn =
        new QPushButton("Search Event");

    QPushButton *editBtn =
        new QPushButton("Edit Event");

    QPushButton *deleteBtn =
        new QPushButton("Delete Event");

    searchBtn->setMinimumHeight(40);
    editBtn->setMinimumHeight(40);
    deleteBtn->setMinimumHeight(40);

    buttonLayout->addWidget(searchBtn);
    buttonLayout->addWidget(editBtn);
    buttonLayout->addWidget(deleteBtn);

    mainLayout->addLayout(buttonLayout);

    QObject::connect(
        searchBtn,
        &QPushButton::clicked,
        [&]() {
            bool ok;

            int eventID = QInputDialog::getInt(
                &window,
                "Search Event",
                "Enter Event ID:",
                1,
                0,
                2147483647,
                1,
                &ok
            );

            if (!ok) {
                return;
            }

            CalendarEvent *event =
                calendar.getEvent(eventID);

            if (event == nullptr) {
                QMessageBox::warning(
                    &window,
                    "Event Not Found",
                    "No event exists with that ID."
                );
                return;
            }

            QString eventInfo =
                "Event ID: " +
                QString::fromStdString(event->getEventID()) +

                "\n\nTitle: " +
                QString::fromStdString(
                    event->getDescription()
                ) +

                "\n\nLocation: " +
                QString::fromStdString(
                    event->getLocation()
                ) +

                "\n\nDate and Time: " +
                toQtDateTime(
                    event->getTime()
                ).toString("MM/dd/yyyy hh:mm AP") +

                "\n\nGet Ready: " +
                QString::number(
                    event->getReadyOff().count()
                ) +
                " minutes before event" +

                "\n\nLeave: " +
                QString::number(
                    event->getLeaveOffset().count()
                ) +
                " minutes before event";

            QMessageBox::information(
                &window,
                "Event Found",
                eventInfo
            );
        }
    );

    QObject::connect(
        editBtn,
        &QPushButton::clicked,
        [&]() {
            bool ok;

            int eventID = QInputDialog::getInt(
                &window,
                "Edit Event",
                "Enter Event ID to edit:",
                1,
                0,
                2147483647,
                1,
                &ok
            );

            if (!ok) {
                return;
            }

            CalendarEvent *event =
                calendar.getEvent(eventID);

            if (event == nullptr) {
                QMessageBox::warning(
                    &window,
                    "Event Not Found",
                    "No event exists with that ID."
                );
                return;
            }

            QDialog dialog(&window);

            dialog.setWindowTitle("Edit Event");
            dialog.resize(400, 350);

            QFormLayout *formLayout =
                new QFormLayout(&dialog);

            QLineEdit *titleInput =
                new QLineEdit(&dialog);

            titleInput->setText(
                QString::fromStdString(
                    event->getDescription()
                )
            );

            QLineEdit *locationInput =
                new QLineEdit(&dialog);

            locationInput->setText(
                QString::fromStdString(
                    event->getLocation()
                )
            );

            QDateTimeEdit *dateTimeInput =
                new QDateTimeEdit(&dialog);

            dateTimeInput->setDisplayFormat(
                "MM/dd/yyyy hh:mm AP"
            );

            dateTimeInput->setCalendarPopup(true);

            dateTimeInput->setDateTime(
                toQtDateTime(event->getTime())
            );

            QSpinBox *readyInput =
                new QSpinBox(&dialog);

            readyInput->setRange(0, 10080);
            readyInput->setSuffix(" minutes");

            readyInput->setValue(
                static_cast<int>(
                    event->getReadyOff().count()
                )
            );

            QSpinBox *leaveInput =
                new QSpinBox(&dialog);

            leaveInput->setRange(0, 10080);
            leaveInput->setSuffix(" minutes");

            leaveInput->setValue(
                static_cast<int>(
                    event->getLeaveOffset().count()
                )
            );

            QDialogButtonBox *buttonBox =
                new QDialogButtonBox(
                    QDialogButtonBox::Save |
                    QDialogButtonBox::Cancel,
                    &dialog
                );

            formLayout->addRow(
                "Event Title:",
                titleInput
            );

            formLayout->addRow(
                "Location:",
                locationInput
            );

            formLayout->addRow(
                "Date and Time:",
                dateTimeInput
            );

            formLayout->addRow(
                "Get Ready Offset:",
                readyInput
            );

            formLayout->addRow(
                "Leave Offset:",
                leaveInput
            );

            formLayout->addRow(buttonBox);

            QObject::connect(
                buttonBox,
                &QDialogButtonBox::accepted,
                &dialog,
                &QDialog::accept
            );

            QObject::connect(
                buttonBox,
                &QDialogButtonBox::rejected,
                &dialog,
                &QDialog::reject
            );

            if (dialog.exec() == QDialog::Accepted) {
                QString newTitle =
                    titleInput->text().trimmed();

                if (newTitle.isEmpty()) {
                    QMessageBox::warning(
                        &window,
                        "Invalid Title",
                        "Event title cannot be empty."
                    );
                    return;
                }

                event->setDescription(
                    newTitle.toStdString()
                );

                event->setLocation(
                    locationInput->text().toStdString()
                );

                event->setEventTime(
                    toChronoTime(
                        dateTimeInput->dateTime()
                    )
                );

                event->setReadyOffset(
                    std::chrono::minutes{
                        readyInput->value()
                    }
                );

                event->setLeaveOffset(
                    std::chrono::minutes{
                        leaveInput->value()
                    }
                );

                QMessageBox::information(
                    &window,
                    "Event Updated",
                    "Event updated successfully!"
                );

                qDebug()
                    << "Event updated:"
                    << eventID;
            }
        }
    );

    QObject::connect(
        deleteBtn,
        &QPushButton::clicked,
        [&]() {
            bool ok;

            int eventID = QInputDialog::getInt(
                &window,
                "Delete Event",
                "Enter Event ID to delete:",
                1,
                0,
                2147483647,
                1,
                &ok
            );

            if (!ok) {
                return;
            }

            CalendarEvent *event =
                calendar.getEvent(eventID);

            if (event == nullptr) {
                QMessageBox::warning(
                    &window,
                    "Event Not Found",
                    "No event exists with that ID."
                );
                return;
            }

            QString eventTitle =
                QString::fromStdString(
                    event->getDescription()
                );

            QMessageBox::StandardButton reply =
                QMessageBox::question(
                    &window,
                    "Confirm Delete",
                    "Are you sure you want to delete:\n" +
                    eventTitle + "?",
                    QMessageBox::Yes |
                    QMessageBox::No
                );

            if (reply != QMessageBox::Yes) {
                return;
            }

            bool success =
                calendar.removeEvent(eventID);

            if (success) {
                QMessageBox::information(
                    &window,
                    "Event Deleted",
                    "Event deleted successfully!"
                );

                updateEventCounter(
                    eventCounter,
                    &calendar
                );

                qDebug()
                    << "Event deleted:"
                    << eventID;
            } else {
                QMessageBox::warning(
                    &window,
                    "Delete Failed",
                    "Could not delete event."
                );
            }
        }
    );

    QObject::connect(
        nextBtn,
        &QPushButton::clicked,
        [&]() {
            currentDisplayDate =
                currentDisplayDate.addMonths(1);

            updateCalendarGrid(
                calendarGrid,
                monthLabel,
                selectedDateLabel,
                &window,
                &calendar,
                eventCounter
            );
        }
    );

    QObject::connect(
        prevBtn,
        &QPushButton::clicked,
        [&]() {
            currentDisplayDate =
                currentDisplayDate.addMonths(-1);

            updateCalendarGrid(
                calendarGrid,
                monthLabel,
                selectedDateLabel,
                &window,
                &calendar,
                eventCounter
            );
        }
    );

    updateCalendarGrid(
        calendarGrid,
        monthLabel,
        selectedDateLabel,
        &window,
        &calendar,
        eventCounter
    );

    updateEventCounter(
        eventCounter,
        &calendar
    );

    window.show();

    return app.exec();
}
