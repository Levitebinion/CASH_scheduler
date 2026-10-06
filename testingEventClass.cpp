#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include "Event.h"
using namespace std;

void printDetails(CalendarEvent test);

int main()
{
    CalendarEvent event(1, "Awesome Party", "My Cool Friend's House", std::chrono::system_clock::now() + std::chrono::hours(48), std::chrono::minutes(30), std::chrono::minutes(20));
    printDetails(event);

    event.setLocation("New place I've never gone before");
    event.setDescription("Even Cooler Party");
    event.setLeaveOffset(chrono::minutes(30));
    event.setReadyOffset(chrono::minutes(45));
    event.setEventTime(std::chrono::system_clock::now() + std::chrono::hours(49));

    printDetails(event);

    return 0;
}

void printDetails(CalendarEvent test) {
    std::time_t eventTimeT =
        std::chrono::system_clock::to_time_t(test.getTime());

    std::time_t readyTimeT =
        std::chrono::system_clock::to_time_t(test.giveReadyTime());

    std::time_t leaveTimeT =
        std::chrono::system_clock::to_time_t(test.giveLeaveTime());

    std::tm eventTm{};
    std::tm readyTm{};
    std::tm leaveTm{};

    localtime_s(&eventTm, &eventTimeT);
    localtime_s(&readyTm, &readyTimeT);
    localtime_s(&leaveTm, &leaveTimeT);

    std::cout << "       \nCALENDAR EVENT\n";
    std::cout << "Event ID: " << test.getEventID() << '\n';
    std::cout << "Description: " << test.getDescription() << '\n';
    std::cout << "Location: " << test.getLocation() << '\n';

    std::cout << "Event Time: "
        << std::put_time(&eventTm, "%Y-%m-%d %I:%M %p")
        << '\n';

    std::cout << "Get Ready Offset: "
        << (test.getReadyOff()).count() << " minutes\n";

    std::cout << "Get Ready Time: "
        << std::put_time(&readyTm, "%Y-%m-%d %I:%M %p")
        << '\n';

    std::cout << "Leave Offset: "
        << (test.getLeaveOffset()).count() << " minutes\n";

    std::cout << "Leave Time: "
        << std::put_time(&leaveTm, "%Y-%m-%d %I:%M %p")
        << '\n';
}