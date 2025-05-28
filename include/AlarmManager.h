// AlarmManager.h
#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>
#include "ntp_clock64.h"

enum class DayMask : uint8_t {
    SUNDAY    = 0x01,
    MONDAY    = 0x02,
    TUESDAY   = 0x04,
    WEDNESDAY = 0x08,
    THURSDAY  = 0x10,
    FRIDAY    = 0x20,
    SATURDAY  = 0x40,
    
    // Common combinations
    WEEKDAYS  = MONDAY | TUESDAY | WEDNESDAY | THURSDAY | FRIDAY,
    WEEKEND   = SATURDAY | SUNDAY,
    DAILY     = 0x7F
};

inline DayMask operator|(DayMask a, DayMask b) {
    return static_cast<DayMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline DayMask operator&(DayMask a, DayMask b) {
    return static_cast<DayMask>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct Alarm {
    uint8_t hour;       // 0-23
    uint8_t minute;     // 0-59
    DayMask days;       // Which days this alarm is active
    bool enabled;       // Is this alarm active
    bool triggered;     // Has this alarm been triggered today
    uint32_t lastTriggerDay; // Day number when last triggered (to reset daily)
};

enum class AlarmEvent {
    NONE,
    TRIGGERED,
    SNOOZED
};

class AlarmManager {
public:
    static constexpr int MAX_ALARMS = 8;
    static constexpr int SNOOZE_MINUTES = 9;

    AlarmManager();
    
    /**
     * @brief Add a new alarm
     * @param hour Hour (0-23)
     * @param minute Minute (0-59) 
     * @param days Day mask for which days alarm is active
     * @return Alarm ID (0-7) or -1 if no slots available
     */
    int addAlarm(uint8_t hour, uint8_t minute, DayMask days);
    
    /**
     * @brief Remove an alarm
     * @param alarmId Alarm ID to remove
     * @return true if successfully removed
     */
    bool removeAlarm(int alarmId);
    
    /**
     * @brief Enable/disable an alarm
     * @param alarmId Alarm ID
     * @param enabled New enabled state
     * @return true if alarm exists and was modified
     */
    bool setAlarmEnabled(int alarmId, bool enabled);
    
    /**
     * @brief Check for alarm triggers and update state
     * @param currentTime Current time from clock
     * @return AlarmEvent indicating what happened
     */
    AlarmEvent update(const ClockDateTime& currentTime);
    
    /**
     * @brief Stop currently active alarm
     */
    void dismissAlarm();
    
    /**
     * @brief Snooze currently active alarm
     */
    void snoozeAlarm();
    
    /**
     * @brief Get currently active alarm ID
     * @return Alarm ID or -1 if none active
     */
    int getActiveAlarm() const { return activeAlarmId_; }
    
    /**
     * @brief Check if any alarm is currently active/ringing
     */
    bool isAlarmActive() const { return activeAlarmId_ != -1; }
    
    /**
     * @brief Get alarm by ID
     * @param alarmId Alarm ID
     * @return Pointer to alarm or nullptr if invalid ID
     */
    const Alarm* getAlarm(int alarmId) const;
    
    /**
     * @brief Get number of configured alarms
     */
    int getAlarmCount() const { return alarmCount_; }

private:
    Alarm alarms_[MAX_ALARMS];
    int alarmCount_;
    int activeAlarmId_;
    uint32_t snoozeUntil_;  // Minutes since midnight when snooze ends
    uint32_t currentDayNumber_; // Track day changes to reset triggered flags
    
    bool shouldTrigger(const Alarm& alarm, const ClockDateTime& time) const;
    uint32_t getDayNumber(const ClockDateTime& time) const;
    uint32_t getMinutesSinceMidnight(const ClockDateTime& time) const;
    DayMask getDayMask(int dayOfWeek) const;
    void resetDailyFlags(const ClockDateTime& time);
};

#endif // ALARM_MANAGER_H