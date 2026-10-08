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
    std::chrono::system_clock::time_point getReadyTime;
    std::chrono::system_clock::time_point leaveTime;

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
		leaveOffset(leaveOffset),
		getReadyTime(alarmTime(eventTime, getReadyOffset)),
		leaveTime(alarmTime(eventTime, leaveOffset))
	{
	}

	// getters/setters
	int getEventID() const {
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

	//changing the event time will update the get ready and leave times
	void setEventTime(std::chrono::system_clock::time_point newTime) {
		eventTime = newTime;
		getReadyTime = alarmTime(eventTime, getReadyOffset);
		leaveTime = alarmTime(eventTime, leaveOffset);
	}

	std::chrono::system_clock::time_point getTime() const {
		return eventTime;
	}

	//changing the getting ready time offset will update the get ready time
	void setReadyOffset(std::chrono::minutes newReadyOff) {
		getReadyOffset = newReadyOff;
		getReadyTime = alarmTime(eventTime, getReadyOffset);
	}

	std::chrono::minutes getReadyOff() const {
		return getReadyOffset;
	}

	//changing the leave time offset will update the leave time
	void setLeaveOffset(std::chrono::minutes newLeaveOff) {
		leaveOffset = newLeaveOff;
		leaveTime = alarmTime(eventTime, leaveOffset);
	}

	std::chrono::minutes getLeaveOffset() const {
		return leaveOffset;
	}
    
    //GET THE READY TIME
	std::chrono::system_clock::time_point giveReadyTime() const {
		return getReadyTime;
	}

    //GET THE LEAVE TIME
	std::chrono::system_clock::time_point giveLeaveTime() const {
		return leaveTime;
	}
	
	// function to determine when to get ready and when to leave
    std::chrono::system_clock::time_point alarmTime(std::chrono::system_clock::time_point eventTime, std::chrono::minutes offset){
        return (eventTime - offset);
    }
	
};
