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

bool alarmRinging = false;
bool ntpConnected = false;  // Track NTP connection status
unsigned long buttonDebounceTime = 0;
static constexpr unsigned long DEBOUNCE_DELAY = 50;

void playerUpdateTask(void *param) {
    while (true) {
        player.update();
        vTaskDelay(pdMS_TO_TICKS(1));
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
                display.showMessage("SNOOZE END", "Wake up!");
                break;
                
            case AlarmEvent::NONE:
            default:
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

enum class ButtonType {
    NONE,
    SNOOZE,
    DISMISS
};

ButtonType readButtonFromSwitches() {
    // Check physical switches for button input
    if (digitalRead(SWITCH_A_PIN) == LOW) {
        return ButtonType::SNOOZE;
    } else if (digitalRead(SWITCH_C_PIN) == LOW) {
        return ButtonType::DISMISS;
    }
    
    return ButtonType::NONE;
}

ButtonType readButtonFromTouch() {
    if (!display.isTouched()) {
        return ButtonType::NONE;
    }
    
    uint16_t x, y;
    display.getTouchPoint(&x, &y);
    
    // Simple touch zones: left half = snooze, right half = dismiss
    if (x < 160) {
        return ButtonType::SNOOZE;
    } else {
        return ButtonType::DISMISS;
    }
}

void handleButtonPress() {
    unsigned long currentTime = millis();
    
    // Debounce check
    if (currentTime - buttonDebounceTime < DEBOUNCE_DELAY) {
        return;
    }
    
    // Check both physical switches and touch input
    ButtonType button = readButtonFromSwitches();
    if (button == ButtonType::NONE) {
        button = readButtonFromTouch();
    }
    
    if (button == ButtonType::NONE) {
        return;
    }
    
    buttonDebounceTime = currentTime;
    
    if (!alarmManager.isAlarmActive() && !alarmRinging) {
        return;
    }
    
    if (button == ButtonType::SNOOZE) {
        alarmManager.snoozeAlarm();
        player.stop();
        vTaskSuspend(playerTaskHandle);
        alarmRinging = false;
        display.showMessage("SNOOZED", "9 minutes");
        delay(2000);
    } else if (button == ButtonType::DISMISS) {
        alarmManager.dismissAlarm();
        player.stop();
        vTaskSuspend(playerTaskHandle);
        alarmRinging = false;
        display.showMessage("DISMISSED", "Good morning!");
        delay(2000);
    }
}

void setupExampleAlarms() {
    // Weekday alarm at 7:00 AM
    alarmManager.addAlarm(7, 0, DayMask::WEEKDAYS);
    
    // Weekend alarm at 9:00 AM
    alarmManager.addAlarm(9, 0, DayMask::WEEKEND);
    
    // Daily reminder at 6:00 PM
    alarmManager.addAlarm(16, 1, DayMask::DAILY);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(1); }

    // Initialize SPI for display and SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
    
    // Initialize I2C for touch controller
    Wire.begin(CTP_SDA_PIN, CTP_SCL_PIN);
    
    // Touch reset sequence
    pinMode(CTP_RST_PIN, OUTPUT);
    digitalWrite(CTP_RST_PIN, HIGH);
    delay(10);
    digitalWrite(CTP_RST_PIN, LOW);
    delay(10);
    digitalWrite(CTP_RST_PIN, HIGH);
    delay(10);
    
    // Touch interrupt pin
    pinMode(CTP_INT_PIN, INPUT);
    
    // Configure external switches
    pinMode(SWITCH_A_PIN, INPUT_PULLUP);
    pinMode(SWITCH_B_PIN, INPUT); // External pull-up
    pinMode(SWITCH_C_PIN, INPUT);
    pinMode(SWITCH_D_PIN, INPUT);
    pinMode(SWITCH_E_PIN, INPUT);
    pinMode(SWITCH_F_PIN, INPUT);
    pinMode(SWITCH_G_PIN, INPUT);
    
    // Initialize display FIRST so we can show status messages
    display.begin(TFT_SPI_FREQ);
    display.showMessage("Initializing...", "Starting up");
    delay(1000);
    
    // Initialize music player
    player.begin();
    display.showMessage("Music Player", "Initialized");
    delay(500);

    // Create player task (starts suspended)
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
    
    // Create alarm task
    xTaskCreatePinnedToCore(
        alarmTask,
        "AlarmTask", 
        4096,
        nullptr,
        2, // Higher priority than player
        &alarmTaskHandle,
        0
    );
    
    display.showMessage("Tasks Created", "Initializing clock");
    delay(500);

    // Initialize clock with timeout protection
    Serial.println("Attempting to initialize NTP clock...");
    display.showMessage("Connecting...", "NTP Server");
    
    // Create a task to handle NTP initialization with timeout
    TaskHandle_t ntpInitTask = nullptr;
    bool ntpTaskCompleted = false;
    
    // Create NTP initialization task
    xTaskCreate([](void* param) {
        bool* completed = (bool*)param;
        try {
            rtclock.begin();
            ntpConnected = true;
            Serial.println("NTP clock initialized successfully");
        } catch (...) {
            ntpConnected = false;
            Serial.println("NTP clock initialization failed");
        }
        *completed = true;
        vTaskDelete(nullptr);
    }, "NTPInit", 4096, &ntpTaskCompleted, 1, &ntpInitTask);
    
    // Wait for completion or timeout
    unsigned long startTime = millis();
    const unsigned long NTP_TIMEOUT = 15000; // 15 seconds timeout
    
    while (!ntpTaskCompleted && (millis() - startTime) < NTP_TIMEOUT) {
        delay(100);
        // Update display with progress
        if ((millis() - startTime) % 2000 < 100) {
            display.showMessage("Connecting...", String((millis() - startTime) / 1000) + "s");
        }
    }
    
    // Clean up the task if it's still running
    if (!ntpTaskCompleted && ntpInitTask != nullptr) {
        vTaskDelete(ntpInitTask);
        Serial.println("NTP initialization timed out");
        display.showMessage("NTP Timeout", "Using offline mode");
        ntpConnected = false;
    }
    
    if (ntpConnected) {
        display.showMessage("NTP Connected", "Time synchronized");
    } else {
        display.showMessage("Offline Mode", "No internet connection");
    }
    
    delay(1000);
    
    // Setup example alarms
    setupExampleAlarms();
    
    display.showMessage("Clock Ready", "Alarms set");
    delay(2000);
    display.setBrightness(200);  // Set comfortable brightness level
    
    Serial.println("Setup complete!");
}

void loop() {
    // Update clock with error handling only if NTP is connected
    if (ntpConnected) {
        try {
            rtclock.update();
        } catch (...) {
            // Continue even if NTP update fails
            ntpConnected = false;
            Serial.println("NTP update failed, switching to offline mode");
        }
    }
    
    // Get current time with fallback
    ClockDateTime* td = nullptr;
    
    if (ntpConnected) {
        try {
            td = new ClockDateTime(rtclock.nowSplit());
        } catch (...) {
            ntpConnected = false;
            Serial.println("NTP time retrieval failed");
        }
    }
    
    if (!ntpConnected || td == nullptr) {
        // Create a basic fallback time structure
        td = new ClockDateTime();
        unsigned long currentMillis = millis();
        td->hour = (currentMillis / 3600000) % 24;
        td->minute = (currentMillis / 60000) % 60;
        td->second = (currentMillis / 1000) % 60;
        td->year = 2024;  // Fallback year
        td->month = 1;    // Fallback month
        td->day = 1;      // Fallback day
    }
    
    // Process alarm commands from serial
    alarmInterface.processCommands();
    
    // Check for button presses (both physical switches and touch)
    handleButtonPress();
    
    // Show time normally, or alarm info if alarm is active
    if (alarmRinging || alarmManager.isAlarmActive()) {
        // Keep showing alarm message
    } else {
        display.showTime(td);
    }
    
    delay(1000);
}