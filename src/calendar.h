#pragma once

#include <chrono>
#include <map>

#include "eventClass.h"

class Calendar {
private:

    // eventID -> CalendarEvent
    std::map<std::string, CalendarEvent> events;

    //private because only adding events should be using it, creates the event's id
     std::string generateEventID(std::chrono::system_clock::time_point eventTime) const;
public:
    // Add an event
    bool addEvent(
        const std::string& description,
        const std::string& location,
        std::chrono::system_clock::time_point eventTime,
        std::chrono::minutes getReadyOffset,
        std::chrono::minutes leaveOffset
    );

    // Remove an event
    bool removeEvent(std::string eventID);

    // Find an event
    CalendarEvent* getEvent(std::string eventID);

    // Check if event exists
    bool eventExists(std::string eventID) const;

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
