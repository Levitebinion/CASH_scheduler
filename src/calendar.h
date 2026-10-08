#pragma once

#include <map>

#include "eventClass.h"

class Calendar {
private:

    // eventID -> CalendarEvent
    std::map<int, CalendarEvent> events;

public:

    // Add an event
    bool addEvent(const CalendarEvent& event);

    // Remove an event
    bool removeEvent(int eventID);

    // Find an event
    CalendarEvent* getEvent(int eventID);

    // Check if event exists
    bool eventExists(int eventID) const;

    // Print all events on console terminal
    void printEvents() const;

    // Number of events
    int getEventCount() const;

    // Print calendar on the console terminal
    void printCalendar(
    std::chrono::year y,
    std::chrono::month m) const;

    // Print current month in a calendar format on console terminal
    void showCurrentMonth() const;
};
