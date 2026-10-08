#include <iostream>
#include <chrono>
#include <string>

#include "Calendar.h"
#include "eventClass.h"

using namespace std;
using namespace std::chrono;

int main() {

    Calendar calendar;

    int choice;

    do {

        cout << "\n";
        cout << "============================\n";
        cout << "         MY CALENDAR\n";
        cout << "============================\n";
        calendar.showCurrentMonth();
        cout << "1. Add Event\n";
        cout << "2. View All Events\n";
        cout << "3. Find Event\n";
        cout << "4. Remove Event\n";
        cout << "5. Exit\n";

        cout << "\nEnter choice: ";
        cin >> choice;

        switch (choice) {

        // ====================================================
        // ADD EVENT
        // ====================================================
        case 1: {

            int eventID;

            string description;
            string location;

            int year;
            int month;
            int day;

            int hour;
            int minute;

            int readyOffset;
            int leaveOffset;


            cout << "\n====== Add Event ======\n";

            cout << "Enter event ID: ";
            cin >> eventID;


            if (calendar.eventExists(eventID)) {

                cout << "An event with that ID already exists.\n";

                break;
            }


            cin.ignore();

            cout << "Enter description: ";
            getline(cin, description);

            cout << "Enter location: ";
            getline(cin, location);


            cout << "Enter year: ";
            cin >> year;

            cout << "Enter month: ";
            cin >> month;

            cout << "Enter day: ";
            cin >> day;

            cout << "Enter hour (0-23): ";
            cin >> hour;

            cout << "Enter minute (0-59): ";
            cin >> minute;


            cout << "Minutes needed to get ready: ";
            cin >> readyOffset;

            cout << "Minutes needed to leave before event: ";
            cin >> leaveOffset;


            // Create the event date
            year_month_day eventDate{
                chrono::year{year},
                chrono::month{
                    static_cast<unsigned>(month)
                },
                chrono::day{
                    static_cast<unsigned>(day)
                }
            };


            if (!eventDate.ok()) {

                cout << "Invalid date.\n";

                break;
            }


            if (
                hour < 0 ||
                hour > 23 ||
                minute < 0 ||
                minute > 59
            ) {

                cout << "Invalid time.\n";

                break;
            }


            // Convert calendar date into a time_point
            sys_days eventDay{
                eventDate
            };


            system_clock::time_point eventTime =
                eventDay
                + hours{hour}
                + minutes{minute};


            CalendarEvent newEvent(
                eventID,
                description,
                location,
                eventTime,
                minutes{readyOffset},
                minutes{leaveOffset}
            );


            if (calendar.addEvent(newEvent)) {

                cout << "\nEvent added successfully!\n";
            }
            else {

                cout << "\nCould not add event.\n";
            }


            break;
        }


        // ====================================================
        // VIEW ALL EVENTS
        // ====================================================
        case 2: {

            cout << "\n====== All Events ======\n";

            calendar.printEvents();

            break;
        }


        // ====================================================
        // FIND EVENT
        // ====================================================
        case 3: {

            int eventID;

            cout << "\nEnter event ID: ";
            cin >> eventID;


            CalendarEvent* event =
                calendar.getEvent(eventID);


            if (event == nullptr) {

                cout << "Event not found.\n";

                break;
            }


            cout << "\n====== Event Found ======\n";

            cout << "Event ID: "
                 << event->getEventID()
                 << "\n";

            cout << "Description: "
                 << event->getDescription()
                 << "\n";

            cout << "Location: "
                 << event->getLocation()
                 << "\n";

            cout << "Ready offset: "
                 << event->getReadyOff().count()
                 << " minutes\n";

            cout << "Leave offset: "
                 << event->getLeaveOffset().count()
                 << " minutes\n";


            break;
        }


        // ====================================================
        // REMOVE EVENT
        // ====================================================
        case 4: {

            int eventID;

            cout << "\nEnter event ID to remove: ";
            cin >> eventID;


            if (calendar.removeEvent(eventID)) {

                cout << "Event removed successfully.\n";
            }
            else {

                cout << "Event not found.\n";
            }


            break;
        }


        // ====================================================
        // EXIT
        // ====================================================
        case 5:

            cout << "Goodbye!\n";

            break;


        default:

            cout << "Invalid choice.\n";
        }


    } while (choice != 5);


    return 0;
}
