#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include "eventClass.h"
#include "Calendar.h"

using namespace std;

int main()
{
    //getting and setting on single event
    CalendarEvent event("1", "Awesome Party", "My Cool Friend's House", std::chrono::system_clock::now() + std::chrono::hours(48), std::chrono::minutes(30), std::chrono::minutes(20));
    event.printDetails();

    event.setLocation("New place I've never gone before");
    event.setDescription("Even Cooler Party");
    event.setLeaveOffset(chrono::minutes(30));
    event.setReadyOffset(chrono::minutes(45));
    event.setEventTime(std::chrono::system_clock::now() + std::chrono::hours(49));

    event.printDetails();


    //adding events to calendar map with new ids
    Calendar calendar;

    //is there a existing json to load
    if (calendar.loadEvents()) {
        std::cout << "Calendar loaded successfully!\n";
    }
    else {
        std::cout << "Starting with an empty calendar.\n";
    }

    auto now = std::chrono::system_clock::now();

    calendar.addEvent(
        "Doctor appointment",
        "Doctor Office Address",
        now + std::chrono::hours(3),
        std::chrono::minutes(30),
        std::chrono::minutes(10)
    );

    calendar.addEvent(
        "Dinner with friends",
        "Nice restaurant",
        now + std::chrono::hours(6),
        std::chrono::minutes(45),
        std::chrono::minutes(20)
    );

    calendar.printEvents();

    calendar.saveEvents();
    return 0;
}
