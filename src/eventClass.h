#pragma once
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <ctime>
#include "Notification.h"

class CalendarEvent {
private:
	std::string eventID; //starts with yyyymmdd and then the number of events created
	std::string description; //what is the event
	std::string location; //where does it take place
	std::chrono::system_clock::time_point eventTime; // what time is the event
	std::chrono::minutes getReadyOffset; //when is it time to get ready ex: 30 minutes before event time
	std::chrono::minutes leaveOffset; // when is it time to leave ex: 10 minutes before event time
  Notification notice;
public:
	// constructor
	CalendarEvent(
		const std::string& eventID,
		const std::string& description,
		const std::string& location,
		std::chrono::system_clock::time_point eventTime,
		std::chrono::minutes getReadyOffset,
		std::chrono::minutes leaveOffset,
    bool noticeOnOff
	)
		: eventID(eventID),
		description(description),
		location(location),
		eventTime(eventTime),
		getReadyOffset(getReadyOffset),
		leaveOffset(leaveOffset),
    notice(noticeOnOff, eventID, getReadyOffset, leaveOffset)
	{
	}

CalendarEvent(
    int eventID,
    const std::string& description,
    const std::string& location,
    std::chrono::system_clock::time_point eventTime,
    std::chrono::minutes getReadyOffset,
    std::chrono::minutes leaveOffset,
    bool noticeOnOff
)
    : CalendarEvent(
        std::to_string(eventID),
        description,
        location,
        eventTime,
        getReadyOffset,
        leaveOffset,
        noticeOnOff
    )
{
}

	// getters/setters
	std::string getEventID() const {
		return eventID;
	}

	void setDescription(std::string newDesc) {
		description = newDesc;
	}

	std::string getDescription() const {
		return description;
	}

	void setLocation(std::string newLoc) {
		location = newLoc;
	}

	std::string getLocation() const {
		return location;
	}

  // Enables or disables the notifications
  void setNotificationsEnabled(bool enabled) {
      notice.setStatus(enabled);
  }

  // Checks the notification onOff status
  bool getNotificationsEnabled() const {
      return notice.getStatus();
  }

  // Calculates and GETS the Ready time
  std::chrono::system_clock::time_point
  getReadyNotificationTime() const {
      return notice.getNotificationTimeForReady(eventTime);
  }

  // Calculates and GETS the Leaving Time
  std::chrono::system_clock::time_point
  getLeavingNotificationTime() const {
      return notice.getNotificationTimeForLeaving(eventTime);
  }

	//changing the event time will update the get ready and leave times
	void setEventTime(std::chrono::system_clock::time_point newTime) {
		if (eventTime != newTime) {
        eventTime = newTime;
        notice.resetNotifications();
    }
	}

	std::chrono::system_clock::time_point getTime() const {
		return eventTime;
	}

	//changing the getting ready time offset will update the get ready time
	void setReadyOffset(std::chrono::minutes newReadyOff) {
		if (getReadyOffset != newReadyOff) {
        getReadyOffset = newReadyOff;
        notice.updateTimeReady(newReadyOff);
    }
	}

	std::chrono::minutes getReadyOff() const {
		return getReadyOffset;
	}

	//changing the leave time offset will update the leave time
	void setLeaveOffset(std::chrono::minutes newLeaveOff) {
		if (leaveOffset != newLeaveOff) {
        leaveOffset = newLeaveOff;
        notice.updateTimeLeaving(newLeaveOff);
    }
	}

	std::chrono::minutes getLeaveOffset() const {
		return leaveOffset;
	}

  // Checks if the ready notification had been set
  bool getReadyNotificationSent() const {
      return notice.wasReadyNotificationSent();
  }

  // Checks if the leaving notification had been set
  bool getLeavingNotificationSent() const {
      return notice.wasLeavingNotificationSent();
  }

  // For json file to save and recover if the notification had been set
  void restoreDeliveryFlags(bool readySent, bool leavingSent) {
      notice.restoreDeliveryFlags(readySent, leavingSent);
  }

  // Checks if the ready notification is on time set by the offset
  bool shouldNotifyReady() {
    return notice.shouldNotifyReady(eventTime);
  }

  // Checks if the leaving notification is on time set by the offset
  bool shouldNotifyLeaving() {
      return notice.shouldNotifyLeaving(eventTime);
  }


	//print all info on the event
	void printDetails() const {
	std::cout << "Event ID: " << eventID << '\n';
	std::cout << "Description: " << description << '\n';
	std::cout << "Location: " << location << '\n';

	auto printTime = [](const char* label,
	std::chrono::system_clock::time_point time) {

		std::time_t timeT =
			std::chrono::system_clock::to_time_t(time);

		std::tm localTime{};

		// Convert to local time
#if defined(_WIN32)
    localtime_s(&localTime, &timeT);
#else
    localtime_r(&timeT, &localTime);
#endif

		//print time with am or pm
		std::cout << label
			<< std::put_time(&localTime, "%b %d, %Y %I:%M %p")
			<< '\n';
	};

	printTime("Event time: ", eventTime);
  printTime("Get ready time: ", getReadyNotificationTime());
  printTime("Leave time: ", getLeavingNotificationTime());

	std::cout << "Get ready offset: "
		<< getReadyOffset.count() << " minutes\n";

	std::cout << "Leave offset: "
		<< leaveOffset.count() << " minutes\n\n";
	}
};
