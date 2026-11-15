#include "Notification.h"

#include <string>
#include <chrono>

  Notification::Notification(
     bool status,
     const std::string& eventNum,
     std::chrono::minutes timeToGo,
     std::chrono::minutes timeToLeave
    )
      : onOff(status),
        eventID(eventNum),
        getReady(timeToGo),
        getLeaving(timeToLeave)
    {
    }

  void Notification::updateTimeReady(std::chrono::minutes newTime) {
    getReady = newTime;
    readyNotificationSent = false;
    }

  void Notification::updateTimeLeaving(std::chrono::minutes newTime) {
    getLeaving = newTime;
    leavingNotificationSent = false;
  }

  void Notification::setStatus(bool newStatus) {
    onOff = newStatus;
  }

  bool Notification::getStatus() const {
    return onOff;
  }

  std::chrono::system_clock::time_point
  Notification::getNotificationTimeForReady(
      std::chrono::system_clock::time_point eventTime
  ) const {
      return eventTime - getReady;
  }

  std::chrono::system_clock::time_point
  Notification::getNotificationTimeForLeaving(
      std::chrono::system_clock::time_point eventTime
  ) const {
      return eventTime - getLeaving;
  }

  bool Notification::shouldNotifyReady(
    std::chrono::system_clock::time_point eventTime
  ) {
      if (!onOff || readyNotificationSent) {
          return false;
      }

      auto currentTime = std::chrono::system_clock::now();

      if (currentTime >= getNotificationTimeForReady(eventTime)
          && currentTime < eventTime) {

          readyNotificationSent = true;
          return true;
      }

      return false;
  }

  bool Notification::shouldNotifyLeaving(
    std::chrono::system_clock::time_point eventTime
  ) {
      if (!onOff || leavingNotificationSent) {
          return false;
      }

      auto currentTime = std::chrono::system_clock::now();

      if (currentTime >= getNotificationTimeForLeaving(eventTime)
          && currentTime < eventTime) {

          leavingNotificationSent = true;
          return true;
      }

      return false;
  }

  void Notification::resetNotifications() {
    readyNotificationSent = false;
    leavingNotificationSent = false;
  }

  bool Notification::wasReadyNotificationSent() const {
      return readyNotificationSent;
  }

  bool Notification::wasLeavingNotificationSent() const {
      return leavingNotificationSent;
  }

  void Notification::restoreDeliveryFlags(
      bool readySent,
      bool leavingSent
  ) {
      readyNotificationSent = readySent;
      leavingNotificationSent = leavingSent;
  }
