// Console test harness for Calendar / CalendarEvent / Notification (C++20).
// The Qt version can eventually replace this worker with a QTimer.
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include "calendar.h"

using namespace std::chrono;

namespace {
std::optional<std::string> askText(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string text;
    if (!std::getline(std::cin, text)) return std::nullopt;
    return text;
}

std::optional<int> askInt(const std::string& prompt) {
    for (;;) {
        auto text = askText(prompt);
        if (!text) return std::nullopt;
        std::istringstream stream(*text);
        int result;
        std::string extra;
        if (stream >> result && !(stream >> extra)) return result;
        std::cout << "Please enter a whole number.\n";
    }
}

// Treat the typed date/time as LOCAL time, not UTC.
std::optional<system_clock::time_point> localEventTime(
    int y, int m, int d, int h, int min
) {
    year_month_day date{year{y}, month{static_cast<unsigned>(m)},
                        day{static_cast<unsigned>(d)}};
    if (!date.ok() || h < 0 || h > 23 || min < 0 || min > 59)
        return std::nullopt;
    std::tm local{};
    local.tm_year = y - 1900;
    local.tm_mon = m - 1;
    local.tm_mday = d;
    local.tm_hour = h;
    local.tm_min = min;
    local.tm_isdst = -1;
    std::time_t timestamp = std::mktime(&local);
    if (timestamp == static_cast<std::time_t>(-1) ||
        local.tm_year != y - 1900 || local.tm_mon != m - 1 ||
        local.tm_mday != d || local.tm_hour != h || local.tm_min != min)
        return std::nullopt;
    return system_clock::from_time_t(timestamp);
}

void menu() {
    std::cout << "\n============ MY CALENDAR ============\n"
              << "1. Add event\n"
              << "2. View all events\n"
              << "3. Find event by ID\n"
              << "4. Remove event\n"
              << "5. Enable/disable event notifications\n"
              << "6. Create a quick notification test event\n"
              << "7. Check notifications now\n"
              << "8. Save events\n"
              << "9. Reload events\n"
              << "0. Save and exit\n";
}
} // namespace

int main() {
    Calendar calendar;
    std::mutex calendarMutex;
    std::cout << (calendar.loadEvents() ? "Loaded events.json.\n"
                                       : "No saved events loaded.\n");

    // The same mutex protects EVERY access to Calendar on both threads.
    std::jthread worker([&](std::stop_token stop) {
        while (!stop.stop_requested()) {
            {
                std::lock_guard<std::mutex> lock(calendarMutex);
                calendar.checkNotifications();
            }
            for (int i = 0; i < 10 && !stop.stop_requested(); ++i)
                std::this_thread::sleep_for(milliseconds{100});
        }
    });

    bool running = true;
    while (running) {
        {
            std::lock_guard<std::mutex> lock(calendarMutex);
            calendar.showCurrentMonth();
        }
        menu();
        auto choice = askInt("Choice: ");
        if (!choice) break;

        switch (*choice) {
        case 1: {
            auto description = askText("Description: ");
            auto location = description ? askText("Location: ") : std::nullopt;
            if (!description || !location) { running = false; break; }
            auto y = askInt("Year (YYYY): ");
            auto m = y ? askInt("Month (1-12): ") : std::nullopt;
            auto d = m ? askInt("Day (1-31): ") : std::nullopt;
            auto h = d ? askInt("Hour (0-23): ") : std::nullopt;
            auto minute = h ? askInt("Minute (0-59): ") : std::nullopt;
            auto ready = minute ? askInt("Minutes before event to get ready: ") : std::nullopt;
            auto leave = ready ? askInt("Minutes before event to leave: ") : std::nullopt;
            if (!leave) { running = false; break; }
            if (*ready < 0 || *leave < 0) {
                std::cout << "Offsets cannot be negative.\n";
                break;
            }
            auto when = localEventTime(*y, *m, *d, *h, *minute);
            if (!when) {
                std::cout << "Invalid local date/time.\n";
                break;
            }
            bool added;
            {
                std::lock_guard<std::mutex> lock(calendarMutex);
                // Current Calendar::addEvent generates a date-based ID.
                added = calendar.addEvent(*description, *location, *when,
                                          minutes{*ready}, minutes{*leave});
            }
            std::cout << (added ? "Event added. View events to find its ID.\n"
                                : "Could not add event.\n");
            break;
        }
        case 2: {
            std::lock_guard<std::mutex> lock(calendarMutex);
            calendar.printEvents();
            break;
        }
        case 3: {
            auto id = askText("Event ID: ");
            if (!id) { running = false; break; }
            std::lock_guard<std::mutex> lock(calendarMutex);
            if (const CalendarEvent* event = calendar.getEvent(*id)) {
                event->printDetails();
                std::cout << "Notifications: "
                          << (event->getNotificationsEnabled() ? "ON" : "OFF") << '\n';
            } else {
                std::cout << "Event not found.\n";
            }
            break;
        }
        case 4: {
            auto id = askText("Event ID to remove: ");
            if (!id) { running = false; break; }
            bool removed;
            {
                std::lock_guard<std::mutex> lock(calendarMutex);
                removed = calendar.removeEvent(*id);
            }
            std::cout << (removed ? "Event removed.\n" : "Event not found.\n");
            break;
        }
        case 5: {
            auto id = askText("Event ID to toggle notifications: ");
            if (!id) { running = false; break; }
            std::lock_guard<std::mutex> lock(calendarMutex);
            if (CalendarEvent* event = calendar.getEvent(*id)) {
                bool enabled = !event->getNotificationsEnabled();
                event->setNotificationsEnabled(enabled);
                std::cout << "Notifications are now " << (enabled ? "ON" : "OFF") << ".\n";
            } else {
                std::cout << "Event not found.\n";
            }
            break;
        }
        case 6: {
            // Event begins in 75 seconds, ready offset=2 min (due now),
            // leave offset=1 min (due in approximately 15 seconds).
            auto when = system_clock::now() + seconds{75};
            bool added;
            {
                std::lock_guard<std::mutex> lock(calendarMutex);
                added = calendar.addEvent("Quick reminder test", "Console",
                                          when, minutes{2}, minutes{1});
            }
            if (added) {
                std::cout << "Test event begins in 75 seconds.\n"
                          << "Get-ready reminder: within 1 second.\n"
                          << "Leaving reminder: about 15 seconds from now.\n"
                          << "The menu remains usable while reminders run.\n";
            } else {
                std::cout << "Could not add test event.\n";
            }
            break;
        }
        case 7: {
            std::lock_guard<std::mutex> lock(calendarMutex);
            calendar.checkNotifications();
            std::cout << "Notification check completed.\n";
            break;
        }
        case 8: {
            std::lock_guard<std::mutex> lock(calendarMutex);
            std::cout << (calendar.saveEvents() ? "Events saved.\n" : "Save failed.\n");
            break;
        }
        case 9: {
            std::lock_guard<std::mutex> lock(calendarMutex);
            std::cout << (calendar.loadEvents() ? "Events reloaded.\n" : "Load failed.\n");
            std::cout << "Notification delivery flags restored from JSON.\n";
            break;
        }
        case 0:
            running = false;
            break;
        default:
            std::cout << "Invalid choice.\n";
        }
    }

    // Stop before saving and before Calendar is destroyed.
    worker.request_stop();
    worker.join();
    {
        std::lock_guard<std::mutex> lock(calendarMutex);
        std::cout << (calendar.saveEvents() ? "Events saved. Goodbye!\n"
                                            : "Warning: save failed. Goodbye!\n");
    }
    return 0;
}
