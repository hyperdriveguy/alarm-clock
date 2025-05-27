// src/main.cpp

#include <Arduino.h>
#include "somebody.h"
#include "MusicPlayer.h"

static constexpr int BUZZER_PINS[3] = { 40, 41, 42 };
static constexpr int LEDC_CH[3] = { 0, 1, 2 };
static constexpr int LEDC_TIMER = 0;
static constexpr int LEDC_RES_BITS = 8;

MusicPlayer player(BUZZER_PINS, LEDC_CH, LEDC_TIMER, LEDC_RES_BITS);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(1); }
    
    player.begin();
    player.play(Ch1, Ch2, Ch3);
}

void loop() {
    player.update();
    
    // Example: Stop after 10 seconds
    static unsigned long start_time = millis();
    if (millis() - start_time > 10000 && player.isPlaying()) {
        player.stop();
        Serial.println("Stopped after 10 seconds");
    }
}