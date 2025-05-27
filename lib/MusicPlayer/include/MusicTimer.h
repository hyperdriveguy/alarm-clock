#ifndef MUSIC_TIMER_H
#define MUSIC_TIMER_H

#include <stdint.h>
#include <esp_timer.h>

class MusicTimer {
 public:
  /** Initializes and starts the timer */
  void begin();

  /** Returns elapsed milliseconds since begin() */
  uint32_t millis() const;

  /** Returns elapsed microseconds since begin() */
  uint64_t micros() const;

  /** Resets the timer to 0 */
  void reset();

 private:
  int64_t _start_us = 0;
};

#endif  // MUSIC_TIMER_H
