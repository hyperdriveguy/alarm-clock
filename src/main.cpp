#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_FT6206.h>
#include <ntp_clock64.h>
#include <MusicPlayer.h>
#include "AlarmManager.h"
#include "AlarmInterface.h"
#include "lvgl_setup.h"
#include <lvgl.h>
#include "song/fireandflames.h"
#include "clock_face.h"
extern lgfx::LGFX_Device tft;  // From lvgl_setup.cpp

// WiFi credentials
static const char* WIFI_SSID     = "optix_legacy";
static const char* WIFI_PASSWORD = "onmyhonor";

// POSIX TZ for US Mountain (auto-DST)
static const char* POSIX_TZ = "MST7MDT,M3.2.0/2,M11.1.0/2";

// Pin definitions
// I²C (Touch) pins
static constexpr int8_t CTP_SDA_PIN = 1;
static constexpr int8_t CTP_SCL_PIN = 2;
static constexpr int8_t CTP_INT_PIN = 10;
static constexpr int8_t CTP_RST_PIN = 13;

// SPI pins for SD and LCD
static constexpr int8_t SD_CS_PIN = 4;
static constexpr int8_t SPI_MISO_PIN = 5; // SDO
static constexpr int8_t SPI_SCK_PIN = 6; // SCK
static constexpr int8_t SPI_MOSI_PIN = 7; // SDI

// LCD control pins (via SPI)
static constexpr int8_t LCD_RS_PIN = 8; // D/C
static constexpr int8_t LCD_RST_PIN = 14; // Reset
static constexpr int8_t LCD_CS_PIN = 18; // Chip-Select
static constexpr int8_t LED_PIN = 9; // Backlight (PWM-capable)

// PWM Piezo Buzzer pins
static constexpr int8_t BUZZER1_PIN = 40;
static constexpr int8_t BUZZER2_PIN = 16;
static constexpr int8_t BUZZER3_PIN = 17; // Shorted (oopsie) so no sound (do not use)
static constexpr int8_t BUZZER4_PIN = 21;

// External Switch pins
static constexpr int8_t SWITCH_A_PIN = 11; // No internal pull-up
static constexpr int8_t SWITCH_B_PIN = 12; // External pull-up
static constexpr int8_t SWITCH_C_PIN = 35; // External pull-up
static constexpr int8_t SWITCH_D_PIN = 36; // External pull-up
static constexpr int8_t SWITCH_E_PIN = 37; // External pull-up
static constexpr int8_t SWITCH_F_PIN = 38; // External pull-up
static constexpr int8_t SWITCH_G_PIN = 39; // External pull-up

// LEDC (PWM) channels for buzzers and backlight
static constexpr int8_t BUZZER1_CH = 3;
static constexpr int8_t BUZZER2_CH = 4;
static constexpr int8_t BUZZER3_CH = 5;
static constexpr int8_t BUZZER4_CH = 6;
static constexpr int8_t BACKLIGHT_CH = 7; // Use channel 7 to avoid conflicts

// Target SPI frequency: 80 MHz
static constexpr uint32_t TFT_SPI_FREQ = 80000000UL;

// Custom SPISettings for 80 MHz, MSB first, SPI mode 0
static SPISettings tftSPISettings(TFT_SPI_FREQ, MSBFIRST, SPI_MODE0);

// NTP Clock setup - sync every hour
NTPClock64 rtclock(WIFI_SSID, WIFI_PASSWORD, POSIX_TZ, "pool.ntp.org", 3600);

// Music player with updated buzzer pins
static constexpr int BUZZER_PINS[3] = { BUZZER1_PIN, BUZZER2_PIN, BUZZER4_PIN };
static constexpr int LEDC_CH[3] = { BUZZER1_CH, BUZZER2_CH, BUZZER4_CH };
static constexpr int LEDC_TIMER = 0;
static constexpr int LEDC_RES_BITS = 8;

MusicPlayer player(BUZZER_PINS, LEDC_CH, LEDC_TIMER, LEDC_RES_BITS);
AlarmManager alarmManager;
AlarmInterface alarmInterface(alarmManager);

TaskHandle_t playerTaskHandle = nullptr;
TaskHandle_t alarmTaskHandle = nullptr;

bool alarmRinging = false;
bool ntpConnected = false;  // Track NTP connection status
unsigned long buttonDebounceTime = 0;
static constexpr unsigned long DEBOUNCE_DELAY = 50;

// Add global volatile flag for button interrupts (0: none, 1: snooze, 2: dismiss)
volatile uint8_t buttonInterruptEvent = 0;

// IRAM_ATTR ISRs for physical button interrupts
IRAM_ATTR void handleSnoozeInterrupt() {
    buttonInterruptEvent = 1;
}
IRAM_ATTR void handleDismissInterrupt() {
    buttonInterruptEvent = 2;
}

// NEW: LVGL messaging helper
void lvgl_showMessage(const char* line1, const char* line2) {
    static lv_obj_t* msg_label1 = nullptr;
    static lv_obj_t* msg_label2 = nullptr;
    if (!msg_label1) {
        msg_label1 = lv_label_create(lv_scr_act());
        lv_obj_align(msg_label1, LV_ALIGN_TOP_MID, 0, 20);
    }
    if (!msg_label2) {
        msg_label2 = lv_label_create(lv_scr_act());
        lv_obj_align(msg_label2, LV_ALIGN_TOP_MID, 0, 60);
    }
    lv_label_set_text(msg_label1, line1);
    lv_label_set_text(msg_label2, line2);
}

// NEW helper: alternate greeting based on time
String getGreeting() {
    ClockDateTime now = rtclock.nowSplit();
    if(now.hour < 12) return "Good morning!";
    else if(now.hour < 17) return "Good afternoon!";
    else if(now.hour < 21) return "Good evening!";
    else return "Good night!";
}

// NEW helper: adjust brightness using LVGL (tft)
void adjustDisplaySettings() {
    ClockDateTime now = rtclock.nowSplit();
    if(now.hour >= 7 && now.hour < 19) {
        tft.setBrightness(255);
    } else {
        tft.setBrightness(100);
    }
    // ...additional LVGL color scheme adjustments...
}

void playerUpdateTask(void *param) {
    while (true) {
        player.update();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// NEW: Task to sync NTP periodically
void ntpSyncTask(void* param) {
    while (true) {
        if(ntpConnected) {
            rtclock.update();
            Serial.println("NTP sync done.");
        }
        vTaskDelay(pdMS_TO_TICKS(3600000));  // every hour
    }
}

void lvglLoopTask(void *param) {
    while (true) {
        lv_timer_handler();  // LVGL internal refresh
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void alarmTask(void *param) {
    while (true) {
        // Use a fallback time source if NTP is not available
        ClockDateTime currentTime;
        
        if (ntpConnected) {
            try {
                currentTime = rtclock.nowSplit();
            } catch (...) {
                ntpConnected = false;  // Mark as disconnected if it fails
                Serial.println("NTP connection lost, switching to fallback");
            }
        }
        
        if (!ntpConnected) {
            // Fallback to system time if NTP fails
            unsigned long currentMillis = millis();
            // This is a very basic fallback - in practice you'd want a better RTC
            currentTime.hour = (currentMillis / 3600000) % 24;
            currentTime.minute = (currentMillis / 60000) % 60;
            currentTime.second = (currentMillis / 1000) % 60;
            // Note: Date components would need proper handling in a real implementation
            currentTime.year = 2024;
            currentTime.month = 1;
            currentTime.day = 1;
        }
        
        AlarmEvent event = alarmManager.update(currentTime);
        
        switch (event) {
            case AlarmEvent::TRIGGERED:
                if (!alarmRinging) {
                    alarmRinging = true;
                    vTaskResume(playerTaskHandle);
                    player.play(Ch1, Ch2, Ch3);
                    lvgl_showMessage("ALARM!", "snooze   |   dismiss");
                }
                break;
                
            case AlarmEvent::SNOOZED:
                alarmRinging = true;
                vTaskResume(playerTaskHandle);
                player.play(Ch1, Ch2, Ch3);
                {
                    // NEW: update countdown display using getSnoozeUntil() from AlarmManager.
                    uint32_t currentMinutes = alarmManager.getMinutesSinceMidnight(currentTime);
                    uint32_t snoozeUntil = alarmManager.getSnoozeUntil();
                    uint32_t remaining = (snoozeUntil + (24*60) - currentMinutes) % (24*60);
                    int mins = remaining;
                    int secs = 0; // For simplicity, seconds are shown as 0.
                    String countdownStr = String("Countdown: ") + String(mins) + ":00";
                    lvgl_showMessage("SNOOZED", countdownStr.c_str());
                }
                break;
                
            case AlarmEvent::NONE:
            default:
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

#include "clock_face.h"

void clockUpdateTask(void *param) {
    while (true) {
        updateClockDisplay();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


enum class ButtonType {
    NONE,
    SNOOZE,
    DISMISS
};

// Modified handleButtonPress: only physical interrupts are used for snooze/dismiss.
void handleButtonPress() {
    if (!alarmRinging) { // Only process button presses when an alarm is playing
        buttonInterruptEvent = 0;
        return;
    }
    if(buttonInterruptEvent != 0) {
        if (buttonInterruptEvent == 1) {  // Snooze (Switch A)
            alarmManager.snoozeAlarm();
            player.stop();
            vTaskSuspend(playerTaskHandle);
            alarmRinging = false;
            lvgl_showMessage("SNOOZED", "Countdown: 9:00");
        } else if (buttonInterruptEvent == 2) {  // Dismiss (Switch C)
            alarmManager.dismissAlarm();
            player.stop();
            vTaskSuspend(playerTaskHandle);
            alarmRinging = false;
            lvgl_showMessage(getGreeting().c_str(), "");
        }
        buttonInterruptEvent = 0;  // clear flag
        delay(2000);
        return;
    }
}

void setupExampleAlarms() {
    // Weekday alarm at 7:00 AM
    alarmManager.addAlarm(7, 0, DayMask::WEEKDAYS);
    
    // // Weekend alarm at 9:00 AM
    // alarmManager.addAlarm(9, 0, DayMask::WEEKEND);
    
    // // Daily reminder at 6:00 PM
    // alarmManager.addAlarm(16, 1, DayMask::DAILY);
}

// NEW: Initialize peripherals including Serial, SPI, I2C, touchscreen, and display.
void initializePeripherals() {
    Serial.begin(115200);
    while (!Serial) { delay(1); }
    // Initialize SPI for display and SD card.
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
    // Initialize I2C for touch controller.
    Wire.begin(CTP_SDA_PIN, CTP_SCL_PIN);
    // Touch reset sequence.
    pinMode(CTP_RST_PIN, OUTPUT);
    digitalWrite(CTP_RST_PIN, HIGH);
    delay(10);
    digitalWrite(CTP_RST_PIN, LOW);
    delay(10);
    digitalWrite(CTP_RST_PIN, HIGH);
    delay(10);
    pinMode(CTP_INT_PIN, INPUT);
    // Configure external switches.
    pinMode(SWITCH_A_PIN, INPUT_PULLUP);
    pinMode(SWITCH_B_PIN, INPUT); // External pull-up
    pinMode(SWITCH_C_PIN, INPUT);
    pinMode(SWITCH_D_PIN, INPUT);
    pinMode(SWITCH_E_PIN, INPUT);
    pinMode(SWITCH_F_PIN, INPUT);
    pinMode(SWITCH_G_PIN, INPUT);
    // Set up button interrupts.
    attachInterrupt(digitalPinToInterrupt(SWITCH_A_PIN), handleSnoozeInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(SWITCH_C_PIN), handleDismissInterrupt, FALLING);
    // Initialize music player.
    player.begin();
    // Optionally show startup message via LVGL:
    lvgl_showMessage("Music Player", "Initialized");
    delay(500);
}

// NEW: Create tasks for player update, alarm, and display.
void initializeTasks() {
    xTaskCreatePinnedToCore(
        playerUpdateTask,
        "PlayerUpdateTask",
        4096,
        nullptr,
        1,
        &playerTaskHandle,
        1
    );
    vTaskSuspend(playerTaskHandle);
    
    xTaskCreatePinnedToCore(
        alarmTask,
        "AlarmTask",
        4096,
        nullptr,
        2,
        &alarmTaskHandle,
        0
    );
    
    xTaskCreatePinnedToCore(
        ntpSyncTask,
        "NTPSyncTask",
        4096,
        nullptr,
        1,
        nullptr,
        0
    );

    xTaskCreatePinnedToCore(lvglLoopTask, "LVGL Loop", 4096, nullptr, 1, nullptr, 1);
    createClockFace();
    xTaskCreatePinnedToCore(clockUpdateTask, "ClockUpdate", 2048, nullptr, 1, nullptr, 1);
    
    lvgl_showMessage("Tasks Created", "Initializing clock");
    delay(500);
}

// NEW: Set WiFi mode, initialize NTP clock, and set up example alarms.
void initializeNTPAndAlarms() {
    WiFi.mode(WIFI_STA);
    Serial.println("WiFi set to station mode.");
    Serial.println("Attempting to initialize NTP clock...");
    lvgl_showMessage("Connecting...", "NTP Server");
    rtclock.begin();
    ntpConnected = true; // assume success if begin() returns
    Serial.println("NTP clock initialized successfully.");
    setupExampleAlarms();
    Serial.println("Example alarms set up.");
    lvgl_showMessage("Clock Ready", "Alarms set");
    tft.setBrightness(200);  // Set comfortable brightness level
    Serial.println("Setup complete!");
}

// Refactored setup() calls helper functions.
void setup() {
    initializePeripherals();
    lvgl_setup();
    initializeTasks();
    initializeNTPAndAlarms();
}

void loop() {
    alarmInterface.processCommands();
    handleButtonPress();
    
    // Adjust brightness and colors based on time (or overridden via Switch B).
    adjustDisplaySettings();
    if(digitalRead(SWITCH_B_PIN) == LOW) {  // Physical override for brightness.
        tft.setBrightness(255);
    }
    
    delay(100);
}