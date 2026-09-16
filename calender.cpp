#include <iostream>
#include <chrono>
#include <format>
#include <map>
#include <vector>
#include <string>

// Define a type alias for our schedule
using CalendarSchedule = std::map<std::chrono::year_month_day, std::vector<std::string>>;

void print_calendar_with_events(std::chrono::year y, std::chrono::month m, const CalendarSchedule& schedule) {
    using namespace std::chrono;

    year_month_day first_day_ymd{y, m, day{1}};
    weekday start_weekday{sys_days{first_day_ymd}};
    year_month_day_last last_day_ymd{y / m / last};
    unsigned total_days = static_cast<unsigned>(last_day_ymd.day());

    // --- 1. Print Calendar Grid Header ---
    std::cout << std::format("\n{:^20}\n", std::format("{} {}", m, y));
    std::cout << "Su Mo Tu We Th Fr Sa\n";

    unsigned spaces = start_weekday.c_encoding(); 
    for (unsigned i = 0; i < spaces; ++i) std::cout << "   ";

    // --- 2. Print Days (with a marker '*' if an event exists) ---
    std::vector<year_month_day> events_this_month;

    for (unsigned d = 1; d <= total_days; ++d) {
        year_month_day current_date{y, m, day{d}};
        
        // Check if there are events scheduled for this day
        bool has_event = schedule.contains(current_date); 
        
        if (has_event) {
            // Print day with an indicator or save it to list below the grid
            std::cout << std::format("{:2}*", d); 
            events_this_month.push_back(current_date);
        } else {
            std::cout << std::format("{:2} ", d);
        }
        
        if ((spaces + d) % 7 == 0) std::cout << "\n";
    }
    std::cout << "\n\n";

    // --- 3. Print Agenda Details Below Grid ---
    if (!events_this_month.empty()) {
        std::cout << "--- Agenda --- \n";
        for (const auto& date : events_this_month) {
            // Retrieve vector of strings for this specific day
            for (const auto& event_text : schedule.at(date)) {
                std::cout << std::format("[Day {:02}]: {}\n", static_cast<unsigned>(date.day()), event_text);
            }
        }
        std::cout << "\n";
    }
}

int main() {
    using namespace std::chrono;

    // Initialize our database of events
    CalendarSchedule schedule;

    // Populate events directly using standard chrono literals
    schedule[2026y / September / 10].push_back("Dentist Appointment");
    schedule[2026y / September / 18].push_back("Project Deadline");
    schedule[2026y / September / 18].push_back("Team Dinner"); // Multiple events on one day
    schedule[2026y / September / 25].push_back("Mom's Birthday");

    // Print calendar for September 2026
    print_calendar_with_events(2026y, September, schedule);

    return 0;
}
