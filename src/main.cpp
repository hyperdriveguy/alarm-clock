// src/main.cpp

#include <Arduino.h>
#include "somebody.h"   // extern const uint8_t Ch1[]

static constexpr int BUZZER_PIN    = 40;
static constexpr int LEDC_CHANNEL  = 0;
static constexpr int LEDC_TIMER    = 0;
static constexpr int LEDC_RES_BITS = 8;

// Game Boy frequency table (two octaves)
static const uint16_t FREQ_REG[25] = {
  0x0000, 0xF82C,0xF89D,0xF907,0xF96B,0xF9CA,0xFA23,0xFA77,0xFAC7,
  0xFB12,0xFB58,0xFB9B,0xFBDA,0xFC16,0xFC4E,0xFC83,0xFCB5,0xFCE5,
  0xFD11,0xFD3B,0xFD63,0xFD89,0xFDAC,0xFDCD,0xFDED
};

// parser state for Channel 1
struct Channel {
  const uint8_t* data = Ch1;
  size_t         idx  = 0;
  uint8_t        octave = 4;
  uint8_t        volume = 128;
  int            ticks_left = 0;
};
static Channel ch;

// global timing
static uint8_t       COMMAND_SIZES[256];
static unsigned long tick_ms   = 100;
static unsigned long last_tick;

// fill COMMAND_SIZES[] per your map
static void initCommandSizes() {
  for (int i = 0; i < 256; i++) COMMAND_SIZES[i] = 1;
  for (int c = 0xC0; c <= 0xCF; c++) COMMAND_SIZES[c] = 4;
  COMMAND_SIZES[0xD2] = 5;
  for (int c = 0xD0; c <= 0xD7; c++) COMMAND_SIZES[c] = 1;
  COMMAND_SIZES[0xD8] = 2;
  COMMAND_SIZES[0xD9] = 2;
  COMMAND_SIZES[0xDA] = 3;
  COMMAND_SIZES[0xDB] = 2;
  COMMAND_SIZES[0xDC] = 2;
  COMMAND_SIZES[0xDD] = 2;
  COMMAND_SIZES[0xDE] = 2;
  COMMAND_SIZES[0xDF] = 1;
  COMMAND_SIZES[0xE0] = 3;
  COMMAND_SIZES[0xE1] = 3;
  COMMAND_SIZES[0xE2] = 2;
  COMMAND_SIZES[0xE3] = 2;
  COMMAND_SIZES[0xE4] = 2;
  COMMAND_SIZES[0xE5] = 2;
  COMMAND_SIZES[0xE6] = 2;
  COMMAND_SIZES[0xEB] = 2;
  COMMAND_SIZES[0xEC] = 1;
  COMMAND_SIZES[0xED] = 1;
  COMMAND_SIZES[0xEE] = 3;
  COMMAND_SIZES[0xEF] = 2;
  COMMAND_SIZES[0xF0] = 2;
  for (int c = 0xF1; c <= 0xF9; c++) COMMAND_SIZES[c] = 1;
  COMMAND_SIZES[0xFF] = 1;
  COMMAND_SIZES[0xFC] = 3;
  COMMAND_SIZES[0xFD] = 4;
  COMMAND_SIZES[0xFE] = 3;
  COMMAND_SIZES[0xFB] = 4;
  COMMAND_SIZES[0xEA] = 3;
}

// Compute GB‑exact frequency
static float getFrequency(uint8_t pitch, uint8_t octave) {
  if (pitch == 0 || pitch > 12) return 0.0f;
  uint8_t baseOct = (octave < 2 ? 0 : 1);
  uint16_t reg    = FREQ_REG[pitch + baseOct * 12] & 0x07FF;
  float    f      = 131072.0f / float(2048 - reg);
  return f * powf(2.0f, float(octave - baseOct));
}

// parse & execute next command for Ch1, with debug
static void processCommand() {
  uint8_t cmd = ch.data[ch.idx++];
  Serial.printf(" cmd=0x%02X @idx=%u ", cmd, ch.idx - 1);

  if (cmd <= 0xCF) {
    uint8_t pitch  = cmd >> 4;
    uint8_t length = cmd & 0x0F;
    ch.ticks_left  = length;
    float freq     = getFrequency(pitch, ch.octave);
    Serial.printf("NOTE p=%u len=%u f=%.1fHz\n", pitch, length, freq);
    ledcWriteTone(LEDC_CHANNEL, uint32_t(freq + 0.5f));
    ledcWrite(LEDC_CHANNEL, ch.volume);
    return;
  }

  if ((cmd & 0xF8) == 0xD0) {
    ch.octave = 0xD7 - cmd;
    Serial.printf("OCTAVE→%u\n", ch.octave);
    return;
  }

  if (cmd == 0xDA) {
    uint16_t t = (uint16_t(ch.data[ch.idx]) << 8) | ch.data[ch.idx+1];
    ch.idx += 2;
    tick_ms = t;
    last_tick = millis();
    Serial.printf("TEMPO→%ums\n", t);
    return;
  }

  if (cmd == 0xDC) {
    ch.volume = ch.data[ch.idx++];
    Serial.printf("VOLUME→%u\n", ch.volume);
    return;
  }

  if (cmd == 0xFF || cmd == 0xEA) {
    ch.idx = 0;
    Serial.println("LOOP");
    return;
  }

  uint8_t sz = COMMAND_SIZES[cmd];
  Serial.printf("SKIP %u bytes\n", sz - 1);
  if (sz > 1) ch.idx += (sz - 1);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); }

  Serial.println("=== Channel 1 Debug ===");
  initCommandSizes();

  // init PWM
  ledcSetup(LEDC_CHANNEL, 1000, LEDC_RES_BITS);
  ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL);

  // prime to first note
  while (ch.ticks_left == 0) {
    processCommand();
  }
  last_tick = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - last_tick < tick_ms) return;
  last_tick = now;

  Serial.printf("\nTick@%lu ticks_left=%d idx=%u\n",
                now, ch.ticks_left, ch.idx);

  if (ch.ticks_left > 0) {
    ch.ticks_left--;
    Serial.printf(" dec→ticks_left=%d\n", ch.ticks_left);
    if (ch.ticks_left > 0) return;
  }
  processCommand();
}
