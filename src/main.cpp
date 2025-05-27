#include <Arduino.h>
#include <MusicPlayer.h>
#include <DisplayInterface.h>
#include "song/somebody.h"

// Create display instance with default pins and settings
DisplayInterface display;

static constexpr int BUZZER_PINS[3] = { 40, 41, 42 };
static constexpr int LEDC_CH[3] = { 0, 1, 2 };
static constexpr int LEDC_TIMER = 0;
static constexpr int LEDC_RES_BITS = 8;

MusicPlayer player(BUZZER_PINS, LEDC_CH, LEDC_TIMER, LEDC_RES_BITS);

TaskHandle_t playerTaskHandle = nullptr;

void playerUpdateTask(void *param) {
    while (true) {
        player.update();
        vTaskDelay(pdMS_TO_TICKS(1));  // Run every 1 ms
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(1); }

    player.begin();

    xTaskCreatePinnedToCore(
        playerUpdateTask,
        "PlayerUpdateTask",
        4096,
        nullptr,
        1,
        &playerTaskHandle,
        1
    );
    
    // Immediately suspend it after creation since music isn't playing yet
    vTaskSuspend(playerTaskHandle);

    display.begin();

    display.showMessage("Booting...", "Please wait");
    delay(2000);
  
    display.showTime("12:34");
    display.showDate("2025-05-27");
  
    delay(3000);
    display.setBrightness(64);  // Dim the backlight
    display.showMessage("Quote:", "Keep it simple.");

    vTaskResume(playerTaskHandle);  // Resume player task to start music playback
    // player.play(SomebodyCh1, SomebodyCh2, SomebodyCh3);
}

void loop() {
    delay(1000);  // Main loop does nothing, player runs in background task
}