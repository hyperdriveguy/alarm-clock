#include <Arduino.h>
#include "pitches.h"
#include "somebody.h"

const int BUZZER_PIN = 42;

/**
 * @brief Compute the duration of a note in milliseconds.
 *
 * Each entry in ch1_tempo[] is “ms per tick” (tempo directive).
 * Each ch1_dur[i] is a tick count.  
 * duration_ms = tempo_ms_per_tick * tick_count
 *
 * @param tempo_ms_per_tick Milliseconds per tick, from ch1_tempo[i].
 * @param ticks             Number of ticks for this note (ch1_dur[i]).
 * @return uint32_t         Duration in ms.
 */
static uint32_t noteDuration(uint16_t tempo_ms_per_tick,
                             uint8_t ticks) {
  return uint32_t(tempo_ms_per_tick) * ticks;
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  for (size_t i = 0; i < RIVALBATTLE_LEN; ++i) {
    uint16_t freq    = ch2_freq[i];     // 0 = rest
    uint8_t  ticks   = ch2_dur[i];      
    uint16_t tp_ms   = ch2_tempo[i];    // ms per tick

    uint32_t dur_ms  = noteDuration(tp_ms, ticks);

    if (freq != 0) {
      tone(BUZZER_PIN, freq, dur_ms);
    }
    delay(dur_ms);
    noTone(BUZZER_PIN);
  }

  // stop after one pass
  while (true) { }
}
