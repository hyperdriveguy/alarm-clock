// AlarmInterface.cpp
#include "AlarmInterface.h"

AlarmInterface::AlarmInterface(AlarmManager& manager) : alarmManager_(manager) {}

void AlarmInterface::processCommands() {
    // nothing to do if no data
    if (!Serial.available()) return;

    // read, echo, and accumulate until newline
    String command;
    while (true) {
        while (!Serial.available()) {
            // wait for next character
        }
        char c = Serial.read();
        Serial.write(c);             // echo immediately
        if (c == '\n') break;        // end of line
        command += c;                // accumulate
    }
    command.trim();
    command.toLowerCase();

    if (command.startsWith("add ")) {
        int hour, minute, dayMask;
        if (sscanf(command.c_str(), "add %d %d %d", &hour, &minute, &dayMask) == 3) {
            int id = alarmManager_.addAlarm(hour, minute, static_cast<DayMask>(dayMask));
            if (id >= 0) {
                Serial.printf("Added alarm %d: %02d:%02d on ", id, hour, minute);
                printDayMask(static_cast<DayMask>(dayMask));
                Serial.println();
            } else {
                Serial.println("Failed to add alarm");
            }
        } else {
            Serial.println("Usage: add HH MM DMASK");
        }
    }
    else if (command.startsWith("remove ")) {
        int id;
        if (sscanf(command.c_str(), "remove %d", &id) == 1) {
            if (alarmManager_.removeAlarm(id)) {
                Serial.printf("Removed alarm %d\n", id);
            } else {
                Serial.println("Invalid alarm ID");
            }
        }
    }
    else if (command.startsWith("enable ")) {
        int id;
        if (sscanf(command.c_str(), "enable %d", &id) == 1) {
            if (alarmManager_.setAlarmEnabled(id, true)) {
                Serial.printf("Enabled alarm %d\n", id);
            } else {
                Serial.println("Invalid alarm ID");
            }
        }
    }
    else if (command.startsWith("disable ")) {
        int id;
        if (sscanf(command.c_str(), "disable %d", &id) == 1) {
            if (alarmManager_.setAlarmEnabled(id, false)) {
                Serial.printf("Disabled alarm %d\n", id);
            } else {
                Serial.println("Invalid alarm ID");
            }
        }
    }
    else if (command == "list") {
        printAlarmList();
    }
    else if (command == "calibrate") {
        Serial.println("Button calibration mode - press buttons to see ADC values:");
        for (int i = 0; i < 20; i++) {
            int adcValue = analogRead(7);
            Serial.printf("ADC: %d\n", adcValue);
            delay(500);
        }
        Serial.println("Calibration complete");
    }
    else if (command == "help") {
        printHelp();
    }
    else {
        Serial.println("Unknown command. Type 'help' for usage.");
    }
}


void AlarmInterface::printHelp() {
    Serial.println("Alarm Commands:");
    Serial.println("  add HH MM DMASK  - Add alarm (24h format)");
    Serial.println("  remove ID        - Remove alarm");
    Serial.println("  enable ID        - Enable alarm");  
    Serial.println("  disable ID       - Disable alarm");
    Serial.println("  list             - List all alarms");
    Serial.println("  calibrate        - Show ADC values for button calibration");
    Serial.println("  help             - Show this help");
    Serial.println("\nDay Masks:");
    Serial.println("  127 = Daily, 62 = Weekdays, 65 = Weekend");
    Serial.println("  Individual: 1=Sun, 2=Mon, 4=Tue, 8=Wed, 16=Thu, 32=Fri, 64=Sat");
}

void AlarmInterface::printAlarmList() {
    int count = alarmManager_.getAlarmCount();
    Serial.printf("Configured alarms (%d):\n", count);
    
    for (int i = 0; i < count; i++) {
        const Alarm* alarm = alarmManager_.getAlarm(i);
        if (alarm) {
            Serial.printf("  [%d] %02d:%02d %s - %s\n", 
                i, alarm->hour, alarm->minute,
                getDayMaskString(alarm->days).c_str(),
                alarm->enabled ? "ON" : "OFF");
        }
    }
    
    if (alarmManager_.isAlarmActive()) {
        Serial.printf("Active alarm: %d\n", alarmManager_.getActiveAlarm());
    }
}

void AlarmInterface::printDayMask(DayMask mask) {
    Serial.print(getDayMaskString(mask));
}

String AlarmInterface::getDayMaskString(DayMask mask) {
    uint8_t m = static_cast<uint8_t>(mask);
    
    if (m == static_cast<uint8_t>(DayMask::DAILY)) return "Daily";
    if (m == static_cast<uint8_t>(DayMask::WEEKDAYS)) return "Weekdays";
    if (m == static_cast<uint8_t>(DayMask::WEEKEND)) return "Weekend";
    
    String result = "";
    if (m & static_cast<uint8_t>(DayMask::SUNDAY)) result += "Su ";
    if (m & static_cast<uint8_t>(DayMask::MONDAY)) result += "Mo ";
    if (m & static_cast<uint8_t>(DayMask::TUESDAY)) result += "Tu ";
    if (m & static_cast<uint8_t>(DayMask::WEDNESDAY)) result += "We ";
    if (m & static_cast<uint8_t>(DayMask::THURSDAY)) result += "Th ";
    if (m & static_cast<uint8_t>(DayMask::FRIDAY)) result += "Fr ";
    if (m & static_cast<uint8_t>(DayMask::SATURDAY)) result += "Sa ";
    
    result.trim();
    return result;
}