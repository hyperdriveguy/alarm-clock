// src/main.cpp

#include <Arduino.h>
#include "somebody.h"   // extern const uint8_t Ch1[], Ch2[], Ch3[]

// ─── Hardware configuration ───────────────────────────────────────────────
static constexpr int BUZZER_PINS[3]  = { 40, 41, 42 };
static constexpr int LEDC_CHANNEL[3] = { 0,  1,  2 };
static constexpr int LEDC_TIMER      = 0;
static constexpr int LEDC_RES_BITS   = 8;

// ─── Game Boy frequency register table (two octaves) ─────────────────────
static const uint16_t FREQ_REG[25] = {
  0x0000,
  0xF82C, 0xF89D, 0xF907, 0xF96B, 0xF9CA, 0xFA23, 0xFA77, 0xFAC7,
  0xFB12, 0xFB58, 0xFB9B, 0xFBDA,
  0xFC16, 0xFC4E, 0xFC83, 0xFCB5, 0xFCE5, 0xFD11, 0xFD3B, 0xFD63,
  0xFD89, 0xFDAC, 0xFDCD, 0xFDED
};

// ─── Per‑channel state ───────────────────────────────────────────────────
struct Channel {
  const uint8_t* data;    // pointer to Ch1/Ch2/Ch3 array
  size_t         idx;     // current read index
  uint8_t        octave;  // 0–7
  uint8_t        volume;  // 0–255
  int            ticks_left;
  int            ledc_ch;
};

// ─── Globals ─────────────────────────────────────────────────────────────
Channel       ch[3];
uint8_t       COMMAND_SIZES[256];
unsigned long tick_ms   = 100;
unsigned long last_tick;

// ─── Initialize COMMAND_SIZES[] based on your Python map ────────────────
static void initCommandSizes() {
  for (int i = 0; i < 256; i++) COMMAND_SIZES[i] = 1;
  for (int c = 0xC0; c <= 0xCF; c++) COMMAND_SIZES[c] = 4;
  COMMAND_SIZES[0xD2] = 5;            // noise
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
  COMMAND_SIZES[0xFF] = 1; // endchannel
  COMMAND_SIZES[0xFC] = 3; // jumpchannel
  COMMAND_SIZES[0xFD] = 4; // loopchannel
  COMMAND_SIZES[0xFE] = 3; // callchannel
  COMMAND_SIZES[0xFB] = 4; // jumpif
  COMMAND_SIZES[0xEA] = 3; // restartchannel
}

// ─── Compute GB‑accurate frequency for pitch & octave ───────────────────
static float getFrequency(uint8_t pitch, uint8_t octave) {
  if (pitch == 0 || pitch > 12) return 0.0f;        // rest or invalid
  uint8_t baseOct = (octave < 2 ? 0 : 1);
  uint16_t reg    = FREQ_REG[pitch + baseOct * 12] & 0x07FF;
  float    f      = 131072.0f / float(2048 - reg);
  return f * powf(2.0f, float(octave - baseOct));
}

// ─── Parse & execute next command for channel c ─────────────────────────
static void processCommand(Channel &c) {
  uint8_t cmd = c.data[c.idx++];
  if (cmd <= 0xCF) {
    // note: hi = pitch, lo = length
    uint8_t pitch  = cmd >> 4;
    uint8_t length = cmd & 0x0F;
    c.ticks_left   = length;
    float freq     = getFrequency(pitch, c.octave);
    if (freq > 0.0f) {
      ledcWriteTone(c.ledc_ch, uint32_t(freq + 0.5f));
      ledcWrite(c.ledc_ch, c.volume);
    } else {
      ledcWriteTone(c.ledc_ch, 0);
    }
    return;
  }
  if ((cmd & 0xF8) == 0xD0) {
    // octave
    c.octave = 0xD7 - cmd;
    return;
  }
  if (cmd == 0xDA) {
    // tempo (big‑endian)
    uint16_t t = (uint16_t(c.data[c.idx]) << 8) | c.data[c.idx + 1];
    c.idx += 2;
    tick_ms = t;
    return;
  }
  if (cmd == 0xDC) {
    // volume/intensity
    c.volume = c.data[c.idx++];
    return;
  }
  if (cmd == 0xFF || cmd == 0xEA) {
    // endchannel or restart → loop back
    c.idx = 0;
    return;
  }
  // skip any other unhandled command
  uint8_t sz = COMMAND_SIZES[cmd];
  if (sz > 1) c.idx += (sz - 1);
}

void setup() {
  initCommandSizes();

  // initialize LEDC and channel state
  for (int i = 0; i < 3; i++) {
    ledcSetup(LEDC_CHANNEL[i], 1000, LEDC_RES_BITS);
    ledcAttachPin(BUZZER_PINS[i], LEDC_CHANNEL[i]);
    ch[i] = {
      (i == 0 ? Ch1 : i == 1 ? Ch2 : Ch3),
      0,    // idx
      4,    // octave
      128,  // volume
      0,    // ticks_left
      LEDC_CHANNEL[i]
    };
  }

  // Prime each channel so they all land on their first note
  for (int i = 0; i < 3; i++) {
    while (ch[i].ticks_left == 0) {
      processCommand(ch[i]);
    }
  }

  last_tick = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - last_tick < tick_ms) return;
  last_tick = now;

  for (int i = 0; i < 3; i++) {
    auto &C = ch[i];
    if (C.ticks_left > 0) {
      if (--C.ticks_left > 0) continue;
    }
    processCommand(C);
  }
}
