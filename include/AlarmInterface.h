// AlarmInterface.h
#ifndef ALARM_INTERFACE_H
#define ALARM_INTERFACE_H

#include "AlarmManager.h"
#include <USBCDC.h>
#include <Arduino.h>

/**
 * @class AlarmInterface
 * @brief Simple USB-CDC interface for managing alarms over native USB
 *
 * Commands:
 * - add HH MM DMASK    : Add alarm (HH=hour, MM=minute, DMASK=day mask)
 * - remove ID          : Remove alarm by ID
 * - enable ID          : Enable alarm
 * - disable ID         : Disable alarm
 * - list               : List all alarms
 * - calibrate          : ADC calibration for buttons
 * - help               : Show commands
 *
 * Day mask examples:
 * - 127 = daily (all days)
 * - 62  = weekdays (M-F)
 * - 65  = weekend (Sat+Sun)
 */
class AlarmInterface {
public:
    /**
     * @brief Construct a new AlarmInterface
     *
     * @param manager Reference to AlarmManager instance
     * @param usb_serial Reference to USBCDC serial interface
     */
    AlarmInterface(AlarmManager& manager, USBCDC& usb_serial);

    /**
     * @brief Process incoming USB-CDC commands, echoing input
     */
    void processCommands();

private:
    AlarmManager& alarmManager_;
    USBCDC& usbSerial_;

    void printHelp();
    void printAlarmList();
    void printDayMask(DayMask mask);
    String getDayMaskString(DayMask mask);
};

#endif // ALARM_INTERFACE_H