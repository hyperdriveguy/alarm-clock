// AlarmManager.cpp
#include "AlarmManager.h"

AlarmManager::AlarmManager() 
    : alarmCount_(0), activeAlarmId_(-1), snoozeUntil_(0), currentDayNumber_(0) {
    // Initialize all alarms as disabled
    for (int i = 0; i < MAX_ALARMS; i++) {
        alarms_[i] = {0, 0, DayMask::DAILY, false, false, 0};
    }
}

int AlarmManager::addAlarm(uint8_t hour, uint8_t minute, DayMask days) {
    if (alarmCount_ >= MAX_ALARMS || hour > 23 || minute > 59) {
        return -1;
    }
    
    int id = alarmCount_++;
    alarms_[id] = {hour, minute, days, true, false, 0};
    return id;
}

bool AlarmManager::removeAlarm(int alarmId) {
    if (alarmId < 0 || alarmId >= alarmCount_) {
        return false;
    }
    
    // If removing active alarm, dismiss it first
    if (activeAlarmId_ == alarmId) {
        dismissAlarm();
    }
    
    // Shift remaining alarms down
    for (int i = alarmId; i < alarmCount_ - 1; i++) {
        alarms_[i] = alarms_[i + 1];
    }
    
    alarmCount_--;
    
    // Adjust active alarm ID if needed
    if (activeAlarmId_ > alarmId) {
        activeAlarmId_--;
    }
    
    return true;
}

bool AlarmManager::setAlarmEnabled(int alarmId, bool enabled) {
    if (alarmId < 0 || alarmId >= alarmCount_) {
        return false;
    }
    
    alarms_[alarmId].enabled = enabled;
    
    // If disabling active alarm, dismiss it
    if (!enabled && activeAlarmId_ == alarmId) {
        dismissAlarm();
    }
    
    return true;
}

AlarmEvent AlarmManager::update(const ClockDateTime& currentTime) {
    resetDailyFlags(currentTime);
    
    uint32_t currentMinutes = getMinutesSinceMidnight(currentTime);
    
    // Check if snooze period is over
    if (snoozeUntil_ > 0 && currentMinutes >= snoozeUntil_) {
        snoozeUntil_ = 0;
        return AlarmEvent::TRIGGERED;
    }
    
    // Check for alarm triggers
    for (int i = 0; i < alarmCount_; i++) {
        if (shouldTrigger(alarms_[i], currentTime)) {
            activeAlarmId_ = i;
            alarms_[i].triggered = true;
            alarms_[i].lastTriggerDay = getDayNumber(currentTime);
            return AlarmEvent::TRIGGERED;
        }
    }
    
    return AlarmEvent::NONE;
}

void AlarmManager::dismissAlarm() {
    activeAlarmId_ = -1;
    snoozeUntil_ = 0;
}

void AlarmManager::snoozeAlarm() {
    if (activeAlarmId_ == -1) return;
    
    // Calculate snooze end time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    uint32_t currentMinutes = timeinfo->tm_hour * 60 + timeinfo->tm_min;
    snoozeUntil_ = currentMinutes + SNOOZE_MINUTES;
    
    // Handle day rollover
    if (snoozeUntil_ >= 24 * 60) {
        snoozeUntil_ -= 24 * 60;
    }
    
    activeAlarmId_ = -1;
}

const Alarm* AlarmManager::getAlarm(int alarmId) const {
    if (alarmId < 0 || alarmId >= alarmCount_) {
        return nullptr;
    }
    return &alarms_[alarmId];
}

bool AlarmManager::shouldTrigger(const Alarm& alarm, const ClockDateTime& time) const {
    // Check if alarm is enabled and not already triggered today
    if (!alarm.enabled || alarm.triggered) {
        return false;
    }
    
    // Check if current day matches alarm days
    DayMask currentDay = getDayMask(time.day % 7); // Assuming day is day of week
    if ((alarm.days & currentDay) == static_cast<DayMask>(0)) {
        return false;
    }
    
    // Check if time matches
    if (alarm.hour != time.hour || alarm.minute != time.minute) {
        return false;
    }
    
    // Don't trigger if in snooze period
    if (snoozeUntil_ > 0) {
        return false;
    }
    
    return true;
}

uint32_t AlarmManager::getDayNumber(const ClockDateTime& time) const {
    // Simple day number calculation (days since epoch)
    return (time.year - 1970) * 365 + (time.year - 1970) / 4 + 
           time.month * 30 + time.day;
}

uint32_t AlarmManager::getMinutesSinceMidnight(const ClockDateTime& time) const {
    return time.hour * 60 + time.minute;
}

DayMask AlarmManager::getDayMask(int dayOfWeek) const {
    // dayOfWeek: 0=Sunday, 1=Monday, ..., 6=Saturday
    switch (dayOfWeek) {
        case 0: return DayMask::SUNDAY;
        case 1: return DayMask::MONDAY;
        case 2: return DayMask::TUESDAY;
        case 3: return DayMask::WEDNESDAY;
        case 4: return DayMask::THURSDAY;
        case 5: return DayMask::FRIDAY;
        case 6: return DayMask::SATURDAY;
        default: return DayMask::SUNDAY;
    }
}

void AlarmManager::resetDailyFlags(const ClockDateTime& time) {
    uint32_t today = getDayNumber(time);
    
    if (today != currentDayNumber_) {
        // New day - reset all triggered flags
        for (int i = 0; i < alarmCount_; i++) {
            alarms_[i].triggered = false;
        }
        currentDayNumber_ = today;
    }
}