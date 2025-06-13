#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <ntp_clock64.h>
#include <MusicPlayer.h>
#include "DisplayInterface.h"
#include "AlarmManager.h"
#include "AlarmInterface.h"
#include "song/fireandflames.h"

// WiFi credentials
static const char* WIFI_SSID     = "optix_legacy";
static const char* WIFI_PASSWORD = "onmyhonor";

// static const char* WIFI_SSID     = "BYUI_Visitor";
// static const char* WIFI_PASSWORD = "";

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
static constexpr int8_t SWITCH_A_PIN = 11; // Internal pull-up
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

// Display and touch instances
Adafruit_ILI9341 tft = Adafruit_ILI9341(LCD_CS_PIN, LCD_RS_PIN, LCD_RST_PIN);
Adafruit_FT6206 touch = Adafruit_FT6206();
DisplayInterface display(&tft, &touch, LED_PIN);

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
TaskHandle_t displayTaskHandle = nullptr;

bool alarmRinging = false;
bool ntpConnected = false;  // Track NTP connection status
bool updateTimeScreen = true;
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

// NEW helper: alternate greeting based on time
String getGreeting() {
    ClockDateTime now = rtclock.nowSplit();
    if(now.hour < 12) return "Good morning!";
    else if(now.hour < 17) return "Good afternoon!";
    else if(now.hour < 21) return "Good evening!";
    else return "Good night!";
}

// NEW helper: adjust brightness and (optionally) color scheme based on time of day
void adjustDisplaySettings() {
    ClockDateTime now = rtclock.nowSplit();
    if(now.hour >=7 && now.hour <19) {
        display.setBrightness(255);
    } else {
        display.setBrightness(100);
    }
    // ...additional color scheme adjustments can be added here...
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

// NEW: Task to handle interactive touchscreen menu
void touchTask(void* param) {
    while(true) {
        if(display.isTouched()){
            uint16_t x, y;
            display.getTouchPoint(&x, &y);
            Serial.printf("Touch at (%d, %d)\n", x, y);
            // Check for lower right area touch (taking rotated display into account)
            if(x > 200 && y < 45) {
                updateTimeScreen = false;  // Disable time screen updates
                display.showMessage("Settings", "Select WiFi");
                delay(2000);
                // ...insert interactive menu logic (e.g. list wifi networks)...
                display.showMessage("Resuming", "");
                delay(500);
                updateTimeScreen = true;  // Re-enable time screen updates
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
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
                    display.showMessage("ALARM!", "snooze   |   dismiss");
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
                    display.showMessage("SNOOZED", String("Countdown: ") + String(mins) + ":00");
                }
                break;
                
            case AlarmEvent::NONE:
            default:
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

void displayTask(void* param) {
    while (true) {
        // Only update if no active alarm
        if (updateTimeScreen && !alarmRinging && !alarmManager.isAlarmActive()) {
            ClockDateTime currentTime = rtclock.nowSplit();
            display.showTime(&currentTime);
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // update display more frequently
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
            display.showMessage("SNOOZED", "Countdown: 9:00");
        } else if (buttonInterruptEvent == 2) {  // Dismiss (Switch C)
            alarmManager.dismissAlarm();
            player.stop();
            vTaskSuspend(playerTaskHandle);
            alarmRinging = false;
            display.showMessage(getGreeting(), "");
        }
        buttonInterruptEvent = 0;  // clear flag
        delay(2000);
        return;
    }
}

void setupExampleAlarms() {
    // Weekday alarm at 7:00 AM
    alarmManager.addAlarm(7, 0, DayMask::WEEKDAYS);
    
    // Weekend alarm at 9:00 AM
    alarmManager.addAlarm(9, 0, DayMask::WEEKEND);
    
    // Daily reminder at 4:00 PM
    // alarmManager.addAlarm(16, 0, DayMask::DAILY);
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
    // Initialize display.
    display.begin(TFT_SPI_FREQ);
    display.showMessage("Initializing...", "Starting up");
    delay(1000);
    // Initialize music player.
    player.begin();
    display.showMessage("Music Player", "Initialized");
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
        displayTask,
        "DisplayTask",
        4096,
        nullptr,
        1,
        &displayTaskHandle,
        1
    );
    // NEW: Create tasks for NTP sync and touchscreen menu handling.
    xTaskCreatePinnedToCore(
        ntpSyncTask,
        "NTPSyncTask",
        4096,
        nullptr,
        1,
        nullptr,
        0
    );
    xTaskCreatePinnedToCore(
        touchTask,
        "TouchTask",
        4096,
        nullptr,
        1,
        nullptr,
        1
    );
    display.showMessage("Tasks Created", "Initializing clock");
    delay(500);
}

// NEW: Set WiFi mode, initialize NTP clock, and set up example alarms.
void initializeNTPAndAlarms() {
    WiFi.mode(WIFI_STA);
    Serial.println("WiFi set to station mode.");
    Serial.println("Attempting to initialize NTP clock...");
    display.showMessage("Connecting...", "NTP Server");
    rtclock.begin();
    ntpConnected = true; // assume success if begin() returns
    Serial.println("NTP clock initialized successfully.");
    setupExampleAlarms();
    Serial.println("Example alarms set up.");
    display.showMessage("Clock Ready", "Alarms set");
    display.setBrightness(200);  // Set comfortable brightness level
    Serial.println("Setup complete!");
}

// Refactored setup() calls helper functions.
void setup() {
    initializePeripherals();
    initializeTasks();
    initializeNTPAndAlarms();
}

void loop() {
    // Removed NTP clock update from loop; NTP sync happens in ntpSyncTask.
    alarmInterface.processCommands();
    handleButtonPress();
    
    // Adjust brightness and colors based on time (or overridden via Switch B).
    adjustDisplaySettings();
    if(digitalRead(SWITCH_B_PIN) == LOW) {  // Physical override for brightness.
        display.setBrightness(255);
    }
    
    delay(100);
}