#pragma once
#include <string>
#include <chrono>

class Notification {
  private:
    bool onOff;
    std::string eventID;
    std::chrono::minutes getReady;
    std::chrono::minutes getLeaving;
    bool readyNotificationSent = false;
    bool leavingNotificationSent = false;

  public:
    Notification(
        bool status,
        const std::string& eventNum,
        std::chrono::minutes timeToGo,
        std::chrono::minutes timeToLeave
    );

    void updateTimeReady(std::chrono::minutes newTime);

    void updateTimeLeaving(std::chrono::minutes newTime);

    // Turns on or off notifications for both ready and leaves time
    void setStatus(bool newStatus);

    // Checks whether notification is on or off
    bool getStatus() const;

    // Calculates the get ready time in actual time Ex: 2:30 pm
    std::chrono::system_clock::time_point getNotificationTimeForReady(
        std::chrono::system_clock::time_point eventTime
    ) const;

    // Calculates the leaving time in actual time Ex: 2:30 pm
    std::chrono::system_clock::time_point getNotificationTimeForLeaving(
        std::chrono::system_clock::time_point eventTime
    ) const;

    // Checks if the Ready notification is activated
    bool shouldNotifyReady(
        std::chrono::system_clock::time_point eventTime
    );

    // Checks if the Leaving notification is activated
    bool shouldNotifyLeaving(
        std::chrono::system_clock::time_point eventTime
    );

    // Resets the Notifications
    void resetNotifications();

    // Check whether each notification has already been sent
    bool wasReadyNotificationSent() const;
    bool wasLeavingNotificationSent() const;

    // Restore notification flags from saved data
    void restoreDeliveryFlags(bool readySent, bool leavingSent);

};
