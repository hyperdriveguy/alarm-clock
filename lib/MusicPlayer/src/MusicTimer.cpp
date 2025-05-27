#include "MusicTimer.h"

void MusicTimer::begin() {
  _start_us = esp_timer_get_time();
}

uint32_t MusicTimer::millis() const {
  return static_cast<uint32_t>((esp_timer_get_time() - _start_us) / 1000);
}

uint64_t MusicTimer::micros() const {
  return esp_timer_get_time() - _start_us;
}

void MusicTimer::reset() {
  _start_us = esp_timer_get_time();
}
