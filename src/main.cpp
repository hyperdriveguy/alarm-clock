// src/main.cpp

#include <Arduino.h>
#include "TestSong.h"   // extern const uint8_t Ch1[], Ch2[], Ch3[]

static constexpr int BUZZER_PINS[3]   = { 40, 41, 42 };
static constexpr int LEDC_CHANNEL[3]  = { 0,  1,  2 };
static constexpr int LEDC_TIMER       = 0;
static constexpr int LEDC_RES_BITS    = 8;

// Game Boy frequency table (two octaves)
static const uint16_t FREQ_REG[25] = {
  0x0000,
  0xF82C,0xF89D,0xF907,0xF96B,0xF9CA,0xFA23,0xFA77,0xFAC7,
  0xFB12,0xFB58,0xFB9B,0xFBDA,
  0xFC16,0xFC4E,0xFC83,0xFCB5,0xFCE5,0xFD11,0xFD3B,0xFD63,
  0xFD89,0xFDAC,0xFDCD,0xFDED
};

struct Channel {
  const uint8_t* data;
  size_t         idx;
  uint8_t        octave;
  uint8_t        volume;
  int            ticks_left;
};

Channel       ch[3];
uint8_t       COMMAND_SIZES[256];
unsigned long tick_ms   = 100;
unsigned long last_tick;

// Populate COMMAND_SIZES[] from your Python map
static void initCommandSizes() {
  for (int i = 0; i < 256; i++) COMMAND_SIZES[i] = 1;
  for (int c = 0xC0; c <= 0xCF; c++) COMMAND_SIZES[c] = 4;
  COMMAND_SIZES[0xD2] = 5;
  for (int c = 0xD0; c <= 0xD7; c++) COMMAND_SIZES[c] = 1;
  COMMAND_SIZES[0xD8] = 2; COMMAND_SIZES[0xD9] = 2;
  COMMAND_SIZES[0xDA] = 3; COMMAND_SIZES[0xDB] = 2;
  COMMAND_SIZES[0xDC] = 2; COMMAND_SIZES[0xDD] = 2;
  COMMAND_SIZES[0xDE] = 2; COMMAND_SIZES[0xDF] = 1;
  COMMAND_SIZES[0xE0] = 3; COMMAND_SIZES[0xE1] = 3;
  COMMAND_SIZES[0xE2] = 2; COMMAND_SIZES[0xE3] = 2;
  COMMAND_SIZES[0xE4] = 2; COMMAND_SIZES[0xE5] = 2;
  COMMAND_SIZES[0xE6] = 2; COMMAND_SIZES[0xEB] = 2;
  COMMAND_SIZES[0xEC] = 1; COMMAND_SIZES[0xED] = 1;
  COMMAND_SIZES[0xEE] = 3; COMMAND_SIZES[0xEF] = 2;
  COMMAND_SIZES[0xF0] = 2;
  for (int c = 0xF1; c <= 0xF9; c++) COMMAND_SIZES[c] = 1;
  COMMAND_SIZES[0xFF] = 1; COMMAND_SIZES[0xFC] = 3;
  COMMAND_SIZES[0xFD] = 4; COMMAND_SIZES[0xFE] = 3;
  COMMAND_SIZES[0xFB] = 4; COMMAND_SIZES[0xEA] = 3;
}

// Compute GB‑accurate frequency
static float getFrequency(uint8_t pitch, uint8_t octave) {
  if (pitch == 0 || pitch > 12) return 0.0f;
  uint8_t baseOct = (octave < 2 ? 0 : 1);
  uint16_t reg    = FREQ_REG[pitch + baseOct * 12] & 0x07FF;
  float    f      = 131072.0f / float(2048 - reg);
  return f * powf(2.0f, float(octave - baseOct));
}

// Parse & execute next command for channel c
static void processCommand(int i) {
  auto &C = ch[i];
  uint8_t cmd = C.data[C.idx++];
  Serial.printf("Ch%d cmd=0x%02X idx=%u ", i+1, cmd, C.idx-1);

  if (cmd <= 0xCF) {
    // note/rest
    uint8_t pitch  = cmd >> 4;
    uint8_t length = cmd & 0x0F;
    C.ticks_left   = length;
    float freq     = getFrequency(pitch, C.octave);
    if (pitch == 0) {
      // rest
      Serial.printf("REST len=%u\n", length);
      ledcWriteTone(LEDC_CHANNEL[i], 0);
      ledcWrite(LEDC_CHANNEL[i], 0);
    } else {
      Serial.printf("NOTE p=%u len=%u f=%.1fHz\n", pitch, length, freq);
      ledcWriteTone(LEDC_CHANNEL[i], uint32_t(freq + 0.5f));
      ledcWrite(LEDC_CHANNEL[i], C.volume);
    }
    return;
  }

  if ((cmd & 0xF8) == 0xD0) {
    // octave
    C.octave = 0xD7 - cmd;
    Serial.printf("OCT→%u\n", C.octave);
    return;
  }

  if (cmd == 0xDA) {
    // tempo
    uint16_t t = (uint16_t(C.data[C.idx]) << 8) | C.data[C.idx+1];
    C.idx += 2;
    tick_ms = t;
    last_tick = millis();
    Serial.printf("TEMPO→%ums\n", t);
    return;
  }

  if (cmd == 0xDC) {
    // volume
    C.volume = C.data[C.idx++];
    Serial.printf("VOL→%u\n", C.volume);
    return;
  }

  if (cmd == 0xFF || cmd == 0xEA) {
    // loop/end
    C.idx = 0;
    Serial.println("LOOP");
    return;
  }

  // skip others
  uint8_t sz = COMMAND_SIZES[cmd];
  Serial.printf("SKIP %u\n", sz-1);
  if (sz > 1) C.idx += (sz - 1);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);
  Serial.println("=== 3‑Channel Debug ===");

  initCommandSizes();
  for (int i = 0; i < 3; i++) {
    // LEDC init
    ledcSetup(LEDC_CHANNEL[i], 1000, LEDC_RES_BITS);
    ledcAttachPin(BUZZER_PINS[i], LEDC_CHANNEL[i]);
    // parser init
    ch[i] = { (i==0?Ch1:(i==1?Ch2:Ch3)), 0, 4, 128, 0 };
  }

  // prime each to first note/rest
  for (int i = 0; i < 3; i++) {
    while (ch[i].ticks_left == 0) {
      processCommand(i);
    }
  }

  last_tick = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - last_tick < tick_ms) return;
  last_tick = now;

  Serial.printf("\n--- Tick @%lu ms ---\n", now);
  for (int i = 0; i < 3; i++) {
    auto &C = ch[i];
    Serial.printf("PreCh%d ticks_left=%d idx=%u\n", i+1, C.ticks_left, C.idx);
    if (C.ticks_left > 0) {
      C.ticks_left--;
      Serial.printf(" dec→%d\n", C.ticks_left);
      if (C.ticks_left == 0) {
        Serial.printf("Ch%d REST end: silencing\n", i+1);
        ledcWriteTone(LEDC_CHANNEL[i], 0);
        ledcWrite(LEDC_CHANNEL[i], 0);
      }
      if (C.ticks_left > 0) continue;
    }
    processCommand(i);
  }
}
