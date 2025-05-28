// AlarmInterface.h
#ifndef ALARM_INTERFACE_H
#define ALARM_INTERFACE_H

#include "AlarmManager.h"

/**
 * @class AlarmInterface
 * @brief Simple serial interface for managing alarms
 * 
 * Commands:
 * - add HH MM DMASK    : Add alarm (HH=hour, MM=minute, DMASK=day mask)
 * - remove ID          : Remove alarm by ID
 * - enable ID          : Enable alarm
 * - disable ID         : Disable alarm  
 * - list               : List all alarms
 * - help               : Show commands
 * 
 * Day mask examples:
 * - 127 = daily (all days)
 * - 62  = weekdays (M-F)
 * - 65  = weekend (Sat+Sun)
 */
class AlarmInterface {
public:
    AlarmInterface(AlarmManager& manager);
    
    /**
     * @brief Process serial commands if available
     */
    void processCommands();
    
private:
    AlarmManager& alarmManager_;
    
    void printHelp();
    void printAlarmList(); 
    void printDayMask(DayMask mask);
    String getDayMaskString(DayMask mask);
};

#endif // ALARM_INTERFACE_H