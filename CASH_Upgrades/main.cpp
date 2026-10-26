#include <QApplication>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSet>
#include <QSpinBox>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <limits>
#include <vector>

struct Event {
    int id = 0;
    QString title;
    QString location;
    QDateTime start;
    int readyMinutes = 60;
    int leaveMinutes = 30;
};

class Scheduler : public QWidget {
    std::vector<Event> events;
    QDate shown = QDate::currentDate();
    QLabel *month = nullptr;
    QLabel *count = nullptr;
    QGridLayout *days = nullptr;
    QListWidget *upcoming = nullptr;
    QSystemTrayIcon *tray = nullptr;
    QSet<QString> delivered;
    bool storageReady = true;

    QString storagePath() const {
        QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (directory.isEmpty()) directory = QDir::homePath() + "/.cash-upgrades";
        QDir().mkpath(directory);
        return directory + "/events.json";
    }

    bool save() {
        QJsonArray entries;
        for (const auto &e : events) {
            QJsonObject obj;
            obj["id"] = e.id;
            obj["title"] = e.title;
            obj["location"] = e.location;
            obj["start"] = e.start.toString(Qt::ISODateWithMs);
            obj["ready"] = e.readyMinutes;
            obj["leave"] = e.leaveMinutes;
            entries.append(obj);
        }
        QSaveFile file(storagePath());
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Save failed", file.errorString());
            return false;
        }
        QByteArray bytes = QJsonDocument(entries).toJson();
        if (file.write(bytes) != bytes.size()) {
            file.cancelWriting();
            QMessageBox::critical(this, "Save failed", "Could not write the event file.");
            return false;
        }
        if (!file.commit()) {
            QMessageBox::critical(this, "Save failed", file.errorString());
            return false;
        }
        return true;
    }

    void load() {
        QFile file(storagePath());
        if (!file.exists()) return;
        if (!file.open(QIODevice::ReadOnly)) {
            storageReady = false;
            QMessageBox::critical(this, "Load failed", file.errorString());
            return;
        }
        QJsonParseError parse;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
        if (parse.error != QJsonParseError::NoError || !doc.isArray()) {
            storageReady = false;
            QMessageBox::critical(this, "Load failed", "Saved data is invalid. No changes will be saved to avoid overwriting it.");
            return;
        }
        std::vector<Event> loaded;
        for (const auto &value : doc.array()) {
            if (!value.isObject()) { storageReady = false; break; }
            QJsonObject obj = value.toObject();
            Event e;
            e.id = obj.value("id").toInt(-1);
            e.title = obj.value("title").toString();
            e.location = obj.value("location").toString();
            e.start = QDateTime::fromString(obj.value("start").toString(), Qt::ISODateWithMs);
            e.readyMinutes = obj.value("ready").toInt(60);
            e.leaveMinutes = obj.value("leave").toInt(30);
            if (e.id < 0 || e.title.isEmpty() || !e.start.isValid() || e.readyMinutes < 0 || e.leaveMinutes < 0 ||
                std::any_of(loaded.begin(), loaded.end(), [&](const Event &other) { return other.id == e.id; })) {
                storageReady = false;
                break;
            }
            loaded.push_back(e);
        }
        if (!storageReady) {
            QMessageBox::critical(this, "Load failed", "Saved events contain invalid entries. Changes are disabled to protect your data.");
            return;
        }
        events = std::move(loaded);
    }

    Event *find(int id) {
        for (auto &e : events) if (e.id == id) return &e;
        return nullptr;
    }

    bool conflict(const Event &candidate, int excluded) const {
        QDateTime candidateEnd = candidate.start.addSecs(3600);
        for (const auto &e : events) {
            if (e.id == excluded) continue;
            if (candidate.start < e.start.addSecs(3600) && e.start < candidateEnd) return true;
        }
        return false;
    }

    QString display(const Event &e) const {
        return QString("#%1  %2  |  %3  |  %4")
            .arg(e.id).arg(e.title).arg(e.start.toString("MMM d, yyyy h:mm AP")).arg(e.location);
    }

    void edit(int id = -1, QDate date = QDate()) {
        if (!storageReady) return;
        Event *existing = id < 0 ? nullptr : find(id);
        if (id >= 0 && !existing) return;
        QDialog dialog(this);
        dialog.setWindowTitle(existing ? "Edit Event" : "Add Event");
        auto *form = new QFormLayout(&dialog);
        auto *eventId = new QSpinBox;
        eventId->setRange(0, std::numeric_limits<int>::max());
        int suggested = 1;
        while (find(suggested) && suggested < std::numeric_limits<int>::max()) ++suggested;
        eventId->setValue(existing ? existing->id : suggested);
        eventId->setEnabled(!existing);
        auto *title = new QLineEdit(existing ? existing->title : QString());
        auto *location = new QLineEdit(existing ? existing->location : QString());
        auto *start = new QDateTimeEdit(existing ? existing->start : QDateTime(date.isValid() ? date : QDate::currentDate(), QTime(9, 0)));
        start->setCalendarPopup(true);
        start->setDisplayFormat("yyyy-MM-dd h:mm AP");
        auto *ready = new QSpinBox;
        ready->setRange(0, 10080);
        ready->setSuffix(" minutes");
        ready->setValue(existing ? existing->readyMinutes : 60);
        auto *leave = new QSpinBox;
        leave->setRange(0, 10080);
        leave->setSuffix(" minutes");
        leave->setValue(existing ? existing->leaveMinutes : 30);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        form->addRow("Event ID", eventId);
        form->addRow("Title", title);
        form->addRow("Location", location);
        form->addRow("Start", start);
        form->addRow("Get ready before", ready);
        form->addRow("Leave before", leave);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        if (title->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Missing title", "Enter an event title.");
            return;
        }
        if (!existing && find(eventId->value())) {
            QMessageBox::warning(this, "Duplicate ID", "That event ID already exists.");
            return;
        }
        Event result{eventId->value(), title->text().trimmed(), location->text(), start->dateTime(), ready->value(), leave->value()};
        if (conflict(result, existing ? existing->id : -1)) {
            if (QMessageBox::question(this, "Time conflict", "This event overlaps another event (assuming each lasts one hour). Save anyway?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        }
        if (existing) *existing = result;
        else events.push_back(result);
        delivered.remove(QString::number(result.id) + ":ready");
        delivered.remove(QString::number(result.id) + ":leave");
        save();
        refresh();
    }

    void eraseEvent() {
        if (!storageReady) return;
        bool ok = false;
        int id = QInputDialog::getInt(this, "Delete Event", "Event ID", 1, 0, std::numeric_limits<int>::max(), 1, &ok);
        if (!ok) return;
        Event *event = find(id);
        if (!event) {
            QMessageBox::warning(this, "Not found", "No event has that ID.");
            return;
        }
        if (QMessageBox::question(this, "Confirm delete", "Delete " + event->title + "?") != QMessageBox::Yes) return;
        events.erase(std::remove_if(events.begin(), events.end(), [&](const Event &e) { return e.id == id; }), events.end());
        delivered.remove(QString::number(id) + ":ready");
        delivered.remove(QString::number(id) + ":leave");
        save();
        refresh();
    }

    void showList(bool search) {
        QString term;
        if (search) {
            bool ok = false;
            term = QInputDialog::getText(this, "Search by name", "Event name contains", QLineEdit::Normal, {}, &ok).trimmed();
            if (!ok || term.isEmpty()) return;
        }
        QDialog dialog(this);
        dialog.setWindowTitle(search ? "Search Results" : "All Events");
        dialog.resize(720, 400);
        auto *layout = new QVBoxLayout(&dialog);
        auto *list = new QListWidget;
        layout->addWidget(list);
        std::vector<Event> sorted = events;
        std::sort(sorted.begin(), sorted.end(), [](const Event &a, const Event &b) { return a.start < b.start; });
        for (const auto &e : sorted) {
            if (search && !e.title.contains(term, Qt::CaseInsensitive)) continue;
            auto *item = new QListWidgetItem(display(e), list);
            item->setData(Qt::UserRole, e.id);
        }
        if (list->count() == 0) list->addItem("No matching events.");
        connect(list, &QListWidget::itemDoubleClicked, &dialog, [this, &dialog](QListWidgetItem *item) {
            if (!item->data(Qt::UserRole).isValid()) return;
            int id = item->data(Qt::UserRole).toInt();
            dialog.accept();
            edit(id);
        });
        dialog.exec();
    }

    void notify() {
        const QDateTime now = QDateTime::currentDateTime();
        for (const auto &e : events) {
            for (const auto &kind : {QString("ready"), QString("leave")}) {
                QDateTime due = e.start.addSecs(-60 * (kind == "ready" ? e.readyMinutes : e.leaveMinutes));
                QString key = QString::number(e.id) + ":" + kind;
                if (due <= now && due.secsTo(now) < 65 && !delivered.contains(key)) {
                    delivered.insert(key);
                    QString message = (kind == "ready" ? "Time to get ready: " : "Time to leave: ") + e.title;
                    if (tray->isVisible()) tray->showMessage("CASH Scheduler", message, QSystemTrayIcon::Information, 10000);
                    else QMessageBox::information(this, "CASH Scheduler", message);
                }
            }
        }
    }

    void refresh() {
        month->setText(shown.toString("MMMM yyyy"));
        count->setText(QString("Total events: %1").arg(events.size()));
        QLayoutItem *item = nullptr;
        while ((item = days->takeAt(0))) {
            delete item->widget();
            delete item;
        }
        QDate first(shown.year(), shown.month(), 1);
        int position = first.dayOfWeek() - 1;
        for (int day = 1; day <= first.daysInMonth(); ++day) {
            QDate date(first.year(), first.month(), day);
            auto *button = new QPushButton(QString::number(day));
            button->setMinimumHeight(42);
            bool occupied = std::any_of(events.begin(), events.end(), [&](const Event &e) { return e.start.date() == date; });
            if (occupied) button->setStyleSheet("background:#b3e5fc;color:#102027;font-weight:bold;");
            if (date == QDate::currentDate()) button->setToolTip("Today");
            else if (occupied) button->setToolTip("Has scheduled events");
            connect(button, &QPushButton::clicked, this, [this, date] { edit(-1, date); });
            days->addWidget(button, position / 7, position % 7);
            ++position;
        }
        upcoming->clear();
        std::vector<Event> sorted = events;
        std::sort(sorted.begin(), sorted.end(), [](const Event &a, const Event &b) { return a.start < b.start; });
        int added = 0;
        for (const auto &e : sorted) {
            if (e.start < QDateTime::currentDateTime()) continue;
            auto *row = new QListWidgetItem(display(e), upcoming);
            row->setData(Qt::UserRole, e.id);
            if (++added == 5) break;
        }
        if (!added) upcoming->addItem("No upcoming events.");
    }

public:
    Scheduler() {
        setWindowTitle("CASH Scheduler - Additional Features");
        resize(920, 730);
        auto *root = new QVBoxLayout(this);
        auto *navigation = new QHBoxLayout;
        auto *previous = new QPushButton("<");
        auto *next = new QPushButton(">");
        auto *today = new QPushButton("Today");
        month = new QLabel;
        month->setAlignment(Qt::AlignCenter);
        navigation->addWidget(previous);
        navigation->addWidget(month, 1);
        navigation->addWidget(next);
        navigation->addWidget(today);
        root->addLayout(navigation);
        auto *weekdays = new QGridLayout;
        const QStringList names = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        for (int i = 0; i < 7; ++i) {
            auto *label = new QLabel(names[i]);
            label->setAlignment(Qt::AlignCenter);
            weekdays->addWidget(label, 0, i);
        }
        root->addLayout(weekdays);
        days = new QGridLayout;
        root->addLayout(days);
        count = new QLabel;
        count->setAlignment(Qt::AlignCenter);
        root->addWidget(count);
        auto *actions = new QHBoxLayout;
        auto *add = new QPushButton("Add Event");
        auto *search = new QPushButton("Search by Name");
        auto *editButton = new QPushButton("Edit by ID");
        auto *remove = new QPushButton("Delete Event");
        auto *all = new QPushButton("View All Events");
        for (auto *button : {add, search, editButton, remove, all}) actions->addWidget(button);
        root->addLayout(actions);
        root->addWidget(new QLabel("Next 5 Upcoming Events (double-click to edit)"));
        upcoming = new QListWidget;
        root->addWidget(upcoming);
        tray = new QSystemTrayIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), this);
        if (QSystemTrayIcon::isSystemTrayAvailable()) tray->show();
        connect(previous, &QPushButton::clicked, this, [this] { shown = shown.addMonths(-1); refresh(); });
        connect(next, &QPushButton::clicked, this, [this] { shown = shown.addMonths(1); refresh(); });
        connect(today, &QPushButton::clicked, this, [this] { shown = QDate::currentDate(); refresh(); });
        connect(add, &QPushButton::clicked, this, [this] { edit(); });
        connect(search, &QPushButton::clicked, this, [this] { showList(true); });
        connect(all, &QPushButton::clicked, this, [this] { showList(false); });
        connect(remove, &QPushButton::clicked, this, [this] { eraseEvent(); });
        connect(editButton, &QPushButton::clicked, this, [this] {
            bool ok = false;
            int id = QInputDialog::getInt(this, "Edit Event", "Event ID", 1, 0, std::numeric_limits<int>::max(), 1, &ok);
            if (!ok) return;
            if (!find(id)) { QMessageBox::warning(this, "Not found", "No event has that ID."); return; }
            edit(id);
        });
        connect(upcoming, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
            if (item->data(Qt::UserRole).isValid()) edit(item->data(Qt::UserRole).toInt());
        });
        load();
        refresh();
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this] { notify(); });
        timer->start(1000);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("CASH Scheduler Team");
    QCoreApplication::setApplicationName("CASH Upgrades");
    Scheduler window;
    window.show();
    return app.exec();
}
