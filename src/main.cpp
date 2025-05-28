#include <Arduino.h>
#include <ntp_clock64.h>
#include <MusicPlayer.h>
#include <DisplayInterface.h>
#include "AlarmManager.h"
#include "AlarmInterface.h"
#include "song/somebody.h"

// static const char* WIFI_SSID     = "optix_legacy";
// static const char* WIFI_PASSWORD = "onmyhonor";

static const char* WIFI_SSID     = "BYUI_Visitor";
static const char* WIFI_PASSWORD = "";

// POSIX TZ for US Mountain (auto-DST)
static const char* POSIX_TZ = "MST7MDT,M3.2.0/2,M11.1.0/2";

// sync every hour
NTPClock64 rtclock(WIFI_SSID, WIFI_PASSWORD, POSIX_TZ, "pool.ntp.org", 3600);

// Create display instance with default pins and settings
DisplayInterface display;

static constexpr int BUZZER_PINS[3] = { 40, 41, 42 };
static constexpr int LEDC_CH[3] = { 0, 1, 2 };
static constexpr int LEDC_TIMER = 0;
static constexpr int LEDC_RES_BITS = 8;

// ADC pin for resistor ladder buttons
static constexpr int BUTTON_ADC_PIN = 7;

// ADC thresholds for button detection (adjust based on your ladder)
static constexpr int SNOOZE_THRESHOLD_LOW = 0;
static constexpr int SNOOZE_THRESHOLD_HIGH = 200;
static constexpr int DISMISS_THRESHOLD_LOW = 300;   // Adjust for your dismiss button
static constexpr int DISMISS_THRESHOLD_HIGH = 600;  // Adjust for your dismiss button

MusicPlayer player(BUZZER_PINS, LEDC_CH, LEDC_TIMER, LEDC_RES_BITS);
AlarmManager alarmManager;
AlarmInterface alarmInterface(alarmManager);

TaskHandle_t playerTaskHandle = nullptr;
TaskHandle_t alarmTaskHandle = nullptr;

bool alarmRinging = false;
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
        ClockDateTime currentTime = rtclock.nowSplit();
        AlarmEvent event = alarmManager.update(currentTime);
        
        switch (event) {
            case AlarmEvent::TRIGGERED:
                if (!alarmRinging) {
                    alarmRinging = true;
                    vTaskResume(playerTaskHandle);
                    player.play(SomebodyCh1, SomebodyCh2, SomebodyCh3);
                    display.showMessage("ALARM!", "Press button");
                }
                break;
                
            case AlarmEvent::SNOOZED:
                alarmRinging = true;
                vTaskResume(playerTaskHandle);
                player.play(SomebodyCh1, SomebodyCh2, SomebodyCh3);
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

ButtonType readButtonFromADC() {
    int adcValue = analogRead(BUTTON_ADC_PIN);
    
    if (adcValue >= SNOOZE_THRESHOLD_LOW && adcValue <= SNOOZE_THRESHOLD_HIGH) {
        return ButtonType::SNOOZE;
    } else if (adcValue >= DISMISS_THRESHOLD_LOW && adcValue <= DISMISS_THRESHOLD_HIGH) {
        return ButtonType::DISMISS;
    }
    
    return ButtonType::NONE;
}

void handleButtonPress() {
    unsigned long currentTime = millis();
    
    // Debounce check
    if (currentTime - buttonDebounceTime < DEBOUNCE_DELAY) {
        return;
    }
    
    ButtonType button = readButtonFromADC();
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

void IRAM_ATTR buttonCheckISR() {
    handleButtonPress();
}

void setupExampleAlarms() {
    // Weekday alarm at 7:00 AM
    alarmManager.addAlarm(7, 0, DayMask::WEEKDAYS);
    
    // Weekend alarm at 9:00 AM
    alarmManager.addAlarm(9, 0, DayMask::WEEKEND);
    
    // Daily reminder at 6:00 PM
    alarmManager.addAlarm(18, 0, DayMask::DAILY);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(1); }

    // Setup ADC for button reading
    analogReadResolution(12);  // 12-bit ADC resolution (0-4095)
    
    player.begin();

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

    rtclock.begin();
    display.begin();
    
    // Setup example alarms
    setupExampleAlarms();
    
    display.showMessage("Clock Ready", "Alarms set");
    delay(2000);
    display.setBrightness(64);
}

void loop() {
    rtclock.update();
    ClockDateTime* td = new ClockDateTime(rtclock.nowSplit());
    
    // Process alarm commands from serial
    alarmInterface.processCommands();
    
    // Check for button presses (polling-based for ADC)
    handleButtonPress();
    
    // Show time normally, or alarm info if alarm is active
    if (alarmRinging || alarmManager.isAlarmActive()) {
        // Keep showing alarm message
    } else {
        display.showTime(td);
    }
    
    delay(1000);
}
