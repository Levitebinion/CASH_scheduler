#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QMessageBox>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QStyle>
#include <QSettings>
#include <QSet>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QColor>
#include <QStringList>
#include <QListWidgetItem>
#include <QUuid>
#include <QCloseEvent>
#include <algorithm>
#include <chrono>
#include "../src/calendar.h"

using namespace std::chrono;

static QDateTime qtTime(system_clock::time_point t) {
    return QDateTime::fromMSecsSinceEpoch(duration_cast<milliseconds>(t.time_since_epoch()).count());
}
static system_clock::time_point chronoTime(const QDateTime &t) {
    return system_clock::time_point(milliseconds(t.toMSecsSinceEpoch()));
}

struct EventOptions {
    int duration = 60;
    QString category = "Other";
    QString priority = "Medium";
    QString repeat = "None";
    int reminder = 0;
    int secondReminder = 0;
    QString seriesId; // shared by recurring occurrences
};

class Scheduler : public QWidget {
    Calendar calendar;
    QSettings settings{"CASH", "Scheduler"};
    QMap<QString, EventOptions> options;
    QSet<QString> notified;
    QDate displayed = QDate::currentDate();
    QGridLayout *daysGrid = nullptr;
    QLabel *monthLabel = nullptr;
    QLabel *counterLabel = nullptr;
    QSystemTrayIcon *tray = nullptr;
    bool trayAvailable = false;

    QString key(const CalendarEvent &event) const { return QString::fromStdString(event.getEventID()); }
    EventOptions details(const QString &id) const { return options.value(id); }
    QString color(const QString &category) const {
        if (category == "School") return "#2879d0";
        if (category == "Work") return "#269b59";
        if (category == "Personal") return "#9856c6";
        if (category == "Important") return "#cf4444";
        return "#d49a00";
    }
    QString label(const CalendarEvent &event) const {
        const auto d = details(key(event));
        return qtTime(event.getTime()).toString("h:mm AP") + "  " +
            QString::fromStdString(event.getDescription()) + "  [" + d.priority + "]";
    }
    QList<QString> eventsOn(const QDate &date) const {
        QList<QString> ids;
        for (const auto &[id, event] : calendar.getEvents())
            if (qtTime(event.getTime()).date() == date) ids.append(QString::fromStdString(id));
        std::sort(ids.begin(), ids.end(), [this](const QString &a, const QString &b) {
            return calendar.getEvent(a.toStdString())->getTime() < calendar.getEvent(b.toStdString())->getTime();
        });
        return ids;
    }
    void loadOptions() {
        settings.beginGroup("eventOptions");
        for (const QString &id : settings.childGroups()) {
            settings.beginGroup(id);
            EventOptions o;
            o.duration = settings.value("duration", 60).toInt();
            o.category = settings.value("category", "Other").toString();
            o.priority = settings.value("priority", "Medium").toString();
            o.repeat = settings.value("repeat", "None").toString();
            o.reminder = settings.value("reminder", 0).toInt();
            o.secondReminder = settings.value("secondReminder", 0).toInt();
            o.seriesId = settings.value("seriesId").toString();
            options[id] = o;
            settings.endGroup();
        }
        settings.endGroup();
    }
    void saveOptions() {
        settings.remove("eventOptions");
        settings.beginGroup("eventOptions");
        for (auto it = options.cbegin(); it != options.cend(); ++it) {
            settings.beginGroup(it.key());
            const auto &o = it.value();
            settings.setValue("duration", o.duration);
            settings.setValue("category", o.category);
            settings.setValue("priority", o.priority);
            settings.setValue("repeat", o.repeat);
            settings.setValue("reminder", o.reminder);
            settings.setValue("secondReminder", o.secondReminder);
            settings.setValue("seriesId", o.seriesId);
            settings.endGroup();
        }
        settings.endGroup();
        settings.sync();
    }
    void saveDelivered() {
        QStringList delivered;
        for (const auto &item : notified) delivered.append(item);
        settings.setValue("deliveredReminders", delivered);
        settings.sync();
    }
    void clearDelivered(const QString &id) {
        for (auto it = notified.begin(); it != notified.end();) {
            if (it->startsWith(id + ":")) it = notified.erase(it);
            else ++it;
        }
        saveDelivered();
    }
    QList<QString> seriesMembers(const QString &series) const {
        QList<QString> ids;
        if (series.isEmpty()) return ids;
        for (const auto &[id, event] : calendar.getEvents()) {
            const QString qid = QString::fromStdString(id);
            if (details(qid).seriesId == series) ids.append(qid);
        }
        return ids;
    }
    void save() {
        if (!calendar.saveEvents())
            QMessageBox::warning(this, "Save failed", "Could not write events.json.");
        saveOptions();
        draw();
    }
    bool conflicts(const QDateTime &start, int duration, const QString &ignore = {}) const {
        const QDateTime end = start.addSecs(duration * 60);
        for (const auto &[id, event] : calendar.getEvents()) {
            const QString existingId = QString::fromStdString(id);
            if (existingId == ignore) continue;
            const QDateTime otherStart = qtTime(event.getTime());
            const QDateTime otherEnd = otherStart.addSecs(details(existingId).duration * 60);
            if (start < otherEnd && otherStart < end) return true;
        }
        return false;
    }
    bool confirmConflict(const QDateTime &start, int duration, const QString &ignore = {}) {
        if (!conflicts(start, duration, ignore)) return true;
        return QMessageBox::question(this, "Schedule conflict",
            "This event overlaps an existing event. Save anyway?",
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    }
    QComboBox *reminderChoice(QWidget *parent, int selected) {
        auto *c = new QComboBox(parent);
        for (int n : {0, 5, 10, 15, 30, 60, 120, 1440})
            c->addItem(n == 0 ? "None" : QString::number(n) + " minutes before", n);
        c->setCurrentIndex(qMax(0, c->findData(selected)));
        return c;
    }
    void edit(const QString &id = {}, QDate date = QDate::currentDate()) {
        CalendarEvent *existing = id.isEmpty() ? nullptr : calendar.getEvent(id.toStdString());
        if (!id.isEmpty() && !existing) return;
        const EventOptions old = details(id);
        QDialog dialog(this);
        dialog.setWindowTitle(existing ? "Edit Event" : "Add Event");
        QFormLayout form(&dialog);
        QLineEdit title(&dialog);
        QTextEdit description(&dialog);
        description.setFixedHeight(65);
        QDateTimeEdit start(&dialog);
        start.setCalendarPopup(true);
        start.setDisplayFormat("MM/dd/yyyy hh:mm AP");
        start.setDateTime(existing ? qtTime(existing->getTime()) : QDateTime(date, QTime(9, 0)));
        QSpinBox duration(&dialog);
        duration.setRange(1, 10080);
        duration.setSuffix(" minutes");
        duration.setValue(old.duration);
        QComboBox category(&dialog);
        category.addItems({"School", "Work", "Personal", "Important", "Other"});
        category.setCurrentText(old.category);
        QComboBox priority(&dialog);
        priority.addItems({"Low", "Medium", "High"});
        priority.setCurrentText(old.priority);
        QComboBox repeat(&dialog);
        repeat.addItems({"None", "Daily", "Weekly", "Monthly"});
        repeat.setCurrentText(old.repeat);
        auto *reminder = reminderChoice(&dialog, old.reminder);
        auto *reminder2 = reminderChoice(&dialog, old.secondReminder);
        if (existing) {
            title.setText(QString::fromStdString(existing->getDescription()));
            description.setPlainText(QString::fromStdString(existing->getLocation()));
        }
        form.addRow("Event Title:", &title);
        form.addRow("Event Description:", &description);
        form.addRow("Start:", &start);
        form.addRow("Duration:", &duration);
        form.addRow("Category:", &category);
        form.addRow("Priority:", &priority);
        form.addRow("Repeat:", &repeat);
        form.addRow("First reminder:", reminder);
        form.addRow("Second reminder:", reminder2);
        QLabel hint("Recurring events are created for the next 12 months.", &dialog);
        form.addRow(&hint);
        QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
        form.addRow(&buttons);
        connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        if (title.text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Missing title", "Enter an event title.");
            return;
        }
        EventOptions o;
        o.duration = duration.value();
        o.category = category.currentText();
        o.priority = priority.currentText();
        o.repeat = repeat.currentText();
        o.reminder = reminder->currentData().toInt();
        o.secondReminder = reminder2->currentData().toInt();
        const QString newTitle = title.text().trimmed();
        const QString newDescription = description.toPlainText();
        if (existing) {
            const QList<QString> members = seriesMembers(old.seriesId);
            bool wholeSeries = false;
            if (members.size() > 1) {
                QMessageBox choice(this);
                choice.setWindowTitle("Edit recurring event");
                choice.setText("Apply these changes to this event or its entire series?");
                auto *one = choice.addButton("This event", QMessageBox::AcceptRole);
                auto *all = choice.addButton("Entire series", QMessageBox::ActionRole);
                choice.addButton(QMessageBox::Cancel);
                choice.exec();
                if (choice.clickedButton() == all) wholeSeries = true;
                else if (choice.clickedButton() != one) return;
            }
            const QDateTime previous = qtTime(existing->getTime());
            const qint64 delta = previous.secsTo(start.dateTime());
            const QList<QString> targets = wholeSeries ? members : QList<QString>{id};
            // Check all changed occurrences before touching any events.
            bool anyConflict = false;
            for (const QString &target : targets) {
                const auto *ev = calendar.getEvent(target.toStdString());
                if (!ev) continue;
                const QDateTime proposed = qtTime(ev->getTime()).addSecs(delta);
                const QDateTime proposedEnd = proposed.addSecs(o.duration * 60);
                for (const auto &[otherId, other] : calendar.getEvents()) {
                    const QString otherKey = QString::fromStdString(otherId);
                    if (targets.contains(otherKey)) continue;
                    const QDateTime otherStart = qtTime(other.getTime());
                    if (proposed < otherStart.addSecs(details(otherKey).duration * 60) &&
                        otherStart < proposedEnd) anyConflict = true;
                }
            }
            if (anyConflict && QMessageBox::question(this, "Schedule conflict",
                "One or more updated events overlap existing events. Save anyway?",
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
            for (const QString &target : targets) {
                auto *ev = calendar.getEvent(target.toStdString());
                if (!ev) continue;
                ev->setDescription(newTitle.toStdString());
                ev->setLocation(newDescription.toStdString());
                ev->setEventTime(chronoTime(qtTime(ev->getTime()).addSecs(delta)));
                EventOptions updated = o;
                updated.seriesId = old.seriesId;
                if (!wholeSeries) updated.repeat = old.repeat;
                options[target] = updated;
                clearDelivered(target);
            }
            // Editing an entire series shifts its occurrences; it doesn't add/remove dates.
        } else {
            QList<QDateTime> proposed;
            QDateTime when = start.dateTime();
            const QDateTime until = when.addYears(1);
            while (when < until && proposed.size() < 366) {
                proposed.append(when);
                if (o.repeat == "Daily") when = when.addDays(1);
                else if (o.repeat == "Weekly") when = when.addDays(7);
                else if (o.repeat == "Monthly") when = when.addMonths(1);
                else break;
            }
            bool anyConflict = false;
            for (const auto &dateTime : proposed) {
                if (conflicts(dateTime, o.duration)) anyConflict = true;
            }
            if (anyConflict && QMessageBox::question(this, "Schedule conflict",
                "One or more occurrences overlap existing events. Create them anyway?",
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
            o.seriesId = proposed.size() > 1 ? QUuid::createUuid().toString(QUuid::WithoutBraces) : QString();
            QList<QString> added;
            for (const auto &dateTime : proposed) {
                QSet<QString> before;
                for (const auto &[oldId, unused] : calendar.getEvents())
                    before.insert(QString::fromStdString(oldId));
                if (!calendar.addEvent(newTitle.toStdString(), newDescription.toStdString(),
                                       chronoTime(dateTime), minutes(0), minutes(0))) {
                    for (const auto &addedId : added) {
                        calendar.removeEvent(addedId.toStdString());
                        options.remove(addedId);
                    }
                    QMessageBox::warning(this, "Event not saved", "Could not create the event series.");
                    return;
                }
                for (const auto &[newId, event] : calendar.getEvents()) {
                    const QString newKey = QString::fromStdString(newId);
                    if (!before.contains(newKey)) {
                        options[newKey] = o;
                        added.append(newKey);
                    }
                }
            }
        }
        save();
    }
    void remove(const QString &id) {
        auto *event = calendar.getEvent(id.toStdString());
        if (!event) return;
        QList<QString> targets{id};
        const auto members = seriesMembers(details(id).seriesId);
        if (members.size() > 1) {
            QMessageBox choice(this);
            choice.setWindowTitle("Delete recurring event");
            choice.setText("Delete only this occurrence or the entire series?");
            auto *one = choice.addButton("This event", QMessageBox::AcceptRole);
            auto *all = choice.addButton("Entire series", QMessageBox::DestructiveRole);
            choice.addButton(QMessageBox::Cancel);
            choice.exec();
            if (choice.clickedButton() == all) targets = members;
            else if (choice.clickedButton() != one) return;
        } else if (QMessageBox::question(this, "Delete event", "Delete " +
            QString::fromStdString(event->getDescription()) + "?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        for (const auto &target : targets) {
            calendar.removeEvent(target.toStdString());
            options.remove(target);
            clearDelivered(target);
        }
        save();
    }
    void openDay(const QDate &date) {
        QDialog dialog(this);
        dialog.setWindowTitle(date.toString("dddd, MMMM d, yyyy"));
        dialog.resize(450, 320);
        QVBoxLayout layout(&dialog);
        QListWidget list(&dialog);
        auto refresh = [&]() {
            list.clear();
            for (const auto &id : eventsOn(date)) {
                const auto *event = calendar.getEvent(id.toStdString());
                if (!event) continue;
                auto *item = new QListWidgetItem(label(*event), &list);
                item->setData(Qt::UserRole, id);
                item->setForeground(QColor(color(details(id).category)));
            }
        };
        refresh();
        layout.addWidget(&list);
        QHBoxLayout actions;
        QPushButton add("Add Event", &dialog), view("View / Edit", &dialog), del("Delete", &dialog);
        actions.addWidget(&add); actions.addWidget(&view); actions.addWidget(&del);
        layout.addLayout(&actions);
        auto selected = [&]() -> QString {
            return list.currentItem() ? list.currentItem()->data(Qt::UserRole).toString() : QString();
        };
        connect(&add, &QPushButton::clicked, &dialog, [&]() { edit({}, date); refresh(); });
        connect(&view, &QPushButton::clicked, &dialog, [&]() {
            const QString id = selected();
            if (!id.isEmpty()) { edit(id); refresh(); }
        });
        connect(&del, &QPushButton::clicked, &dialog, [&]() {
            const QString id = selected();
            if (!id.isEmpty()) { remove(id); refresh(); }
        });
        connect(&list, &QListWidget::itemDoubleClicked, &dialog, [&](QListWidgetItem *item) {
            const QString id = item->data(Qt::UserRole).toString();
            edit(id); refresh();
        });
        dialog.exec();
    }
    void draw() {
        monthLabel->setText(displayed.toString("MMMM yyyy"));
        counterLabel->setText("Total Events: " + QString::number(calendar.getEventCount()));
        while (auto *item = daysGrid->takeAt(0)) {
            delete item->widget();
            delete item;
        }
        const QDate first(displayed.year(), displayed.month(), 1);
        for (int day = 1; day <= displayed.daysInMonth(); ++day) {
            const QDate date(displayed.year(), displayed.month(), day);
            const auto ids = eventsOn(date);
            auto *button = new QPushButton(QString::number(day), this);
            button->setMinimumSize(65, 53);
            if (!ids.isEmpty()) {
                const QString id = ids.first();
                const QString starColor = date < QDate::currentDate() ? "#888888" : color(details(id).category);
                button->setText(QString::number(day) + " ★");
                button->setStyleSheet("QPushButton { color: " + starColor + "; font-weight: bold; }");
                QStringList names;
                for (const QString &eventId : ids)
                    names << QString::fromStdString(calendar.getEvent(eventId.toStdString())->getDescription());
                button->setToolTip(names.join("\n"));
            }
            daysGrid->addWidget(button, (first.dayOfWeek() - 1 + day - 1) / 7,
                               (first.dayOfWeek() - 1 + day - 1) % 7);
            connect(button, &QPushButton::clicked, this, [this, date]() { openDay(date); });
        }
    }
    void checkReminders() {
        const QDateTime now = QDateTime::currentDateTime();
        for (const auto &[idStd, event] : calendar.getEvents()) {
            const QString id = QString::fromStdString(idStd);
            const QDateTime start = qtTime(event.getTime());
            QSet<int> reminderOffsets{details(id).reminder, details(id).secondReminder};
            for (int minutesBefore : reminderOffsets) {
                if (minutesBefore <= 0 || now >= start || now < start.addSecs(-minutesBefore * 60)) continue;
                const QString notificationId = id + ":" + QString::number(start.toMSecsSinceEpoch()) +
                                               ":" + QString::number(minutesBefore);
                if (notified.contains(notificationId)) continue;
                notified.insert(notificationId);
                saveDelivered();
                const QString message = QString::fromStdString(event.getDescription()) +
                    " starts at " + start.toString("hh:mm AP");
                QApplication::beep();
                if (trayAvailable) tray->showMessage("CASH Reminder", message, QSystemTrayIcon::Information, 10000);
                else QMessageBox::information(this, "CASH Reminder", message);
            }
        }
    }
public:
    Scheduler() {
        setWindowTitle("CASH Scheduler");
        resize(750, 640);
        calendar.loadEvents();
        loadOptions();
        for (const auto &delivered : settings.value("deliveredReminders").toStringList())
            notified.insert(delivered);
        // Keep only delivery records for events that still exist.
        for (auto it = notified.begin(); it != notified.end();) {
            if (!calendar.eventExists(it->section(':', 0, 0).toStdString())) it = notified.erase(it);
            else ++it;
        }
        saveDelivered();
        auto *root = new QVBoxLayout(this);
        auto *header = new QHBoxLayout;
        auto *prev = new QPushButton("<", this);
        auto *next = new QPushButton(">", this);
        monthLabel = new QLabel(this);
        monthLabel->setAlignment(Qt::AlignCenter);
        header->addWidget(prev); header->addWidget(monthLabel, 1); header->addWidget(next);
        root->addLayout(header);
        auto *weekday = new QGridLayout;
        const QStringList names = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        for (int i = 0; i < names.size(); ++i) {
            auto *day = new QLabel(names[i], this);
            day->setAlignment(Qt::AlignCenter);
            weekday->addWidget(day, 0, i);
        }
        root->addLayout(weekday);
        daysGrid = new QGridLayout;
        root->addLayout(daysGrid);
        counterLabel = new QLabel(this);
        counterLabel->setAlignment(Qt::AlignCenter);
        root->addWidget(counterLabel);
        auto *hint = new QLabel("Click a day to view, add, edit, or delete events. Stars show event categories.", this);
        hint->setAlignment(Qt::AlignCenter);
        root->addWidget(hint);
        connect(prev, &QPushButton::clicked, this, [this]() { displayed = displayed.addMonths(-1); draw(); });
        connect(next, &QPushButton::clicked, this, [this]() { displayed = displayed.addMonths(1); draw(); });
        tray = new QSystemTrayIcon(style()->standardIcon(QStyle::SP_ComputerIcon), this);
        trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
        if (trayAvailable) tray->show();
        auto *reminderTimer = new QTimer(this);
        connect(reminderTimer, &QTimer::timeout, this, [this]() { checkReminders(); });
        reminderTimer->start(15000);
        auto *dayTimer = new QTimer(this);
        connect(dayTimer, &QTimer::timeout, this, [this]() { draw(); });
        dayTimer->start(60000);
        draw();
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QFile styles(QCoreApplication::applicationDirPath() + "/styles.qss");
    if (!styles.open(QFile::ReadOnly | QFile::Text)) {
        styles.setFileName("styles.qss");
        styles.open(QFile::ReadOnly | QFile::Text);
    }
    if (styles.isOpen()) app.setStyleSheet(QString::fromUtf8(styles.readAll()));
    Scheduler window;
    window.show();
    return app.exec();
}
