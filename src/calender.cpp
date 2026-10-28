#include "calendar.h"

#include <iostream>
#include <format>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace std;
using namespace std::chrono;

void Calendar::printEvents() const {
    if (events.empty()) {
        std::cout << "No events in the calendar.\n";
        return;
    }

    for (const auto& [id, event] : events) {
        std::cout << "Event Details:\n";
        event.printDetails();
    }
    std::cout << "\n";
}

string Calendar::generateEventID(system_clock::time_point eventTime) const {
    const auto timeT = system_clock::to_time_t(eventTime); //convert timepoint to time_t to get year,month,date out
    tm localTime{}; //hold converted date

    //this checks what time conversion to use, windows or linux/mac
    //read some stuff that says it should be here so it is
    #if defined(_WIN32)
        localtime_s(&localTime, &timeT);
    #else
        localtime_r(&timeT, &localTime);
    #endif

    //makes date portion of ID, YYYYMMDD
    const string date = format(
        "{:04}{:02}{:02}",
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday
    );

    // make second part of the id
     int largestIDnum = 1; //in case no other events that day, start at 1

    //search events map for events with IDs matching the date
    for (const auto& [id, event] : events) {
        if (id.size() <= 8 || !id.starts_with(date)) { //skip ids that don't match date or aren't long enough to be valid
            continue;
        }
        //get everything after the date portion
        const string suffix = id.substr(8);

        // Ensure the suffix contains only digits.
        if (!ranges::all_of(suffix, [](unsigned char c) {
            return c >= '0' && c <= '9';
        })) {
            continue;
        }
        //convert the suffix portion to an integer to do some math
        const int number = std::stoi(suffix);
        //track the largest suffix found plus one so that we avoid reusing id numbers
        largestIDnum = max(largestIDnum, number + 1);
    }

    //Combine date and suffix number into full id
    //i think it's reasonable to assume there will not be more than 999 events on a single day
    return format("{}{:03}", date, largestIDnum);
}


bool Calendar::addEvent(
    const std::string& description,
    const std::string& location,
    std::chrono::system_clock::time_point eventTime,
    std::chrono::minutes getReadyOffset,
    std::chrono::minutes leaveOffset
) {
    // Generate a unique ID for this event
    std::string id = generateEventID(eventTime);

    // Construct the event directly inside the map
    auto [it, inserted] = events.try_emplace(
        id,
        id,
        description,
        location,
        eventTime,
        getReadyOffset,
        leaveOffset
    );

    return inserted;
}

bool Calendar::addEvent(const CalendarEvent& event) {
    return events.try_emplace(event.getEventID(), event).second;
}

bool Calendar::removeEvent(string eventID) {

    if (!events.contains(eventID)) {
        return false;
    }

    events.erase(eventID);

    return true;
}

bool Calendar::removeEvent(int eventID) {
    return removeEvent(std::to_string(eventID));
}

CalendarEvent* Calendar::getEvent(string eventID) {

    auto it = events.find(eventID);

    if (it == events.end()) {
        return nullptr;
    }

    return &it->second;
}

CalendarEvent* Calendar::getEvent(int eventID) {
    return getEvent(std::to_string(eventID));
}

const CalendarEvent* Calendar::getEvent(string eventID) const {
    auto it = events.find(eventID);

    if (it == events.end()) {
        return nullptr;
    }

    return &it->second;
}

const CalendarEvent* Calendar::getEvent(int eventID) const {
    return getEvent(std::to_string(eventID));
}

bool Calendar::eventExists(string eventID) const {

    return events.contains(eventID);
}

bool Calendar::eventExists(int eventID) const {
    return eventExists(std::to_string(eventID));
}

int Calendar::getEventCount() const {

    return static_cast<int>(
        events.size()
    );
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
