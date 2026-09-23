#pragma once
#include <string>
#include <chrono>

class CalendarEvent {
private:
	int eventID;
	std::string description; //what is the event
	std::string location; //where does it take place
	std::chrono::system_clock::time_point eventTime; // what time is the event
	std::chrono::minutes getReadyOffset; //when is it time to get ready ex: 30 minutes before event time
	std::chrono::minutes leaveOffset; // when is it time to leave ex: 10 minutes before event time

public:
	// constructor
	CalendarEvent(
		int eventID,
		const std::string& description,
		const std::string& location,
		std::chrono::system_clock::time_point eventTime,
		std::chrono::minutes getReadyOffset,
		std::chrono::minutes leaveOffset
	)
		: eventID(eventID),
		description(description),
		location(location),
		eventTime(eventTime),
		getReadyOffset(getReadyOffset),
		leaveOffset(leaveOffset)
	{
	}

	// getters/setters
	int getEventID() const {
		return eventID;
	} //don't think eventID should need to be changed? depends on how we organize things


	//set and get the description of the event
	void setDescription(std::string newDesc) {
		description = newDesc;
	}

	std::string getDescription() const {
		return description;
	}


	//set and get the location of the event
	void setLocation(std::string newLoc) {
		location = newLoc;
	}

	std::string getLocation() const {
		return location;
	}


	//set and get the time of the event
	void setEventTime(std::chrono::system_clock::time_point newTime) {
		eventTime = newTime;
	}

	std::chrono::system_clock::time_point getTime() const {
		return eventTime;
	}


	//set and get the amount of time before event for the start getting ready alarm
	void setReadyOffset(std::chrono::minutes newReadyOff) {
		getReadyOffset = newReadyOff;
	}

	std::chrono::minutes getReadyOff() const {
		return getReadyOffset;
	}


	//set and get the amount of time before event for the leave now alarm
	void setLeaveOffset(std::chrono::minutes newLeaveOff) {
		leaveOffset = newLeaveOff;
	}

	std::chrono::minutes getLeaveOffset() const {
		return leaveOffset;
	}
	

	// functions to determine when to get ready and when to leave
	
};