#include "Calendar.h"

#include <iostream>
#include <format>
#include <chrono>
#include <vector>

using namespace std;
using namespace std::chrono;


bool Calendar::addEvent(const CalendarEvent& event) {

    int id = event.getEventID();

    // Don't allow duplicate IDs
    if (events.contains(id)) {
        return false;
    }

    events.emplace(id, event);

    return true;
}

bool Calendar::removeEvent(int eventID) {

    if (!events.contains(eventID)) {
        return false;
    }

    events.erase(eventID);

    return true;
}

CalendarEvent* Calendar::getEvent(int eventID) {

    auto it = events.find(eventID);

    if (it == events.end()) {
        return nullptr;
    }

    return &it->second;
}


bool Calendar::eventExists(int eventID) const {

    return events.contains(eventID);
}

int Calendar::getEventCount() const {

    return static_cast<int>(
        events.size()
    );
}

void Calendar::printEvents() const {

    if (events.empty()) {

        cout << "No events scheduled.\n";

        return;
    }

    for (const auto& [id, event] : events) {

        time_t eventTime =
            system_clock::to_time_t(event.getTime());

        tm* localTime =
            std::localtime(&eventTime);

        cout << "----------------------------\n";

        cout << "Event ID: "
             << event.getEventID()
             << "\n";

        cout << "Description: "
             << event.getDescription()
             << "\n";

        cout << "Location: "
             << event.getLocation()
             << "\n";

        cout << format(
            "Date: {}/{}/{}\n",
            localTime->tm_mon + 1,
            localTime->tm_mday,
            localTime->tm_year + 1900
        );

        int hour =
            localTime->tm_hour % 12;

        if (hour == 0)
            hour = 12;

        string amPm =
            localTime->tm_hour >= 12
            ? "PM"
            : "AM";

        cout << format(
            "Time: {}:{:02} {}\n",
            hour,
            localTime->tm_min,
            amPm
        );
    }
  }

  void Calendar::printCalendar(
    std::chrono::year y,
    std::chrono::month m
) const {

    year_month_day first_day_ymd{
        y,
        m,
        day{1}
    };

    weekday start_weekday{
        sys_days{first_day_ymd}
    };

    year_month_day_last last_day_ymd{
        y / m / last
    };

    unsigned total_days =
        static_cast<unsigned>(
            last_day_ymd.day()
        );

    cout << format(
        "\n{:^21}\n",
        format("{} {}", m, y)
    );

    cout << "Su Mo Tu We Th Fr Sa\n";

    unsigned spaces =
        start_weekday.c_encoding();

    for (unsigned i = 0; i < spaces; ++i) {
        cout << "   ";
    }

    for (unsigned d = 1; d <= total_days; ++d) {

        year_month_day current_date{
            y,
            m,
            day{d}
        };

        bool has_event = false;

        for (const auto& [id, event] : events) {

            time_t eventTime =
                system_clock::to_time_t(
                    event.getTime()
                );

            tm* localTime =
                std::localtime(
                    &eventTime
                );

            year_month_day event_date{
                year{
                    localTime->tm_year + 1900
                },
                month{
                    static_cast<unsigned>(
                        localTime->tm_mon + 1
                    )
                },
                day{
                    static_cast<unsigned>(
                        localTime->tm_mday
                    )
                }
            };

            if (event_date == current_date) {
                has_event = true;
                break;
            }
        }

        if (has_event) {
            cout << format("{:2}*", d);
        }
        else {
            cout << format("{:2} ", d);
        }

        if ((spaces + d) % 7 == 0) {
            cout << '\n';
        }
    }

    cout << "\n\n";
    cout << "------- Agenda -------\n";

    bool found_event = false;

    for (const auto& [id, event] : events) {

        time_t eventTime =
            system_clock::to_time_t(
                event.getTime()
            );

        tm* localTime =
            std::localtime(
                &eventTime
            );

        year event_year{
            localTime->tm_year + 1900
        };

        month event_month{
            static_cast<unsigned>(
                localTime->tm_mon + 1
            )
        };

        if (event_year != y ||
            event_month != m) {
            continue;
        }

        found_event = true;

        int display_hour =
            localTime->tm_hour % 12;

        if (display_hour == 0) {
            display_hour = 12;
        }

        string am_pm =
            localTime->tm_hour >= 12
            ? "PM"
            : "AM";

        cout << format(
            "[Day {:02}] {}:{:02} {} - {}\n",
            localTime->tm_mday,
            display_hour,
            localTime->tm_min,
            am_pm,
            event.getDescription()
        );
    }

    if (!found_event) {
        cout << "No events scheduled.\n";
    }
}

  void Calendar::showCurrentMonth() const {

    auto now =
        system_clock::now();

    time_t currentTime =
        system_clock::to_time_t(now);

    tm* localTime =
        std::localtime(&currentTime);

    year currentYear{
        localTime->tm_year + 1900
    };

    month currentMonth{
        static_cast<unsigned>(
            localTime->tm_mon + 1
        )
    };

    printCalendar(
        currentYear,
        currentMonth
    );
}
