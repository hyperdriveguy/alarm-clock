// main.cpp

#include <Arduino.h>
#include "somebody.h"   // provides extern const uint8_t Ch1[], Ch2[], Ch3[]

// GPIO pins
static constexpr int BUZZER_PINS[3] = { 40, 41, 42 };

// LEDC channels (0–15 on ESP32‑S3)
static constexpr int LEDC_CHANNEL[3] = { 0, 1, 2 };
static constexpr int LEDC_TIMER    = 0;      // use timer 0
static constexpr int LEDC_RES_BITS = 8;      // 8‑bit duty resolution

// frequency table for note (0=C_,1=C#,…12=B_) at octave 4 as base (A4=440Hz)
static const float NOTE_FREQ[12] = {
  261.63, 277.18, 293.66, 311.13, 329.63, 349.23,
  369.99, 392.00, 415.30, 440.00, 466.16, 493.88
};

struct Channel {
  const uint8_t* data;
  size_t idx;
  uint8_t  octave;
  uint8_t  volume;
  int      ticks_left;
  int      ledc_ch;
};

static Channel ch[3];
static unsigned long tick_ms = 100;   // default 100 ms

//----------------------------------------------------------------------------
// @brief  Compute frequency in Hz for given pitch code and octave.
// @param  pitch  0=C_, …, 11=B_,  and 0 also represents rest when len>0
// @param  octave 0–7
// @return frequency in Hz (0 for rest)
//----------------------------------------------------------------------------
static float getFrequency(uint8_t pitch, uint8_t octave) {
  if (pitch >= 12) return 0.0f;
  float base = NOTE_FREQ[pitch];
  return base * powf(2.0f, float(octave - 4));
}

void setup() {
  for (int i = 0; i < 3; i++) {
    // init LEDC PWM
    ledcSetup(LEDC_CHANNEL[i], 1000, LEDC_RES_BITS);
    ledcAttachPin(BUZZER_PINS[i], LEDC_CHANNEL[i]);

    // init parser state
    ch[i].data       = (i==0 ? Ch1 : (i==1 ? Ch2 : Ch3));
    ch[i].idx        = 0;
    ch[i].octave     = 4;
    ch[i].volume     = 128;
    ch[i].ticks_left = 0;
    ch[i].ledc_ch    = LEDC_CHANNEL[i];
  }
}

void processCommand(Channel &c) {
  uint8_t cmd = c.data[c.idx++];
  if (cmd <= 0xCF) {
    // note command: upper nibble = pitch, lower = length
    uint8_t pitch  = cmd >> 4;
    uint8_t length = cmd & 0x0F;
    c.ticks_left   = length;
    float freq     = getFrequency(pitch, c.octave);
    if (freq > 0.0f) {
      ledcWriteTone(c.ledc_ch, uint32_t(freq + 0.5f));
      ledcWrite(c.ledc_ch, c.volume);
    } else {
      // rest
      ledcWriteTone(c.ledc_ch, 0);
    }
  }
  else if ((cmd & 0xF8) == 0xD0) {
    // octave: cmd = 0xD7 - n
    c.octave = 0xD7 - cmd;
  }
  else if (cmd == 0xDA) {
    // tempo: two‐byte big endian
    uint8_t hi = c.data[c.idx++];
    uint8_t lo = c.data[c.idx++];
    tick_ms = (unsigned)hi << 8 | lo;
  }
  else if (cmd == 0xDC) {
    // volume/intensity
    c.volume = c.data[c.idx++];
  }
  else {
    // skip parameters of unhandled commands
    // use COMMAND_SIZES from your Python code to know how many bytes to skip
    // e.g. if(cmd==0xE5) idx+=1; etc.
    // for brevity we ignore others here
  }
}

void loop() {
  static unsigned long last_tick = millis();
  unsigned long now = millis();
  if (now - last_tick < tick_ms) return;
  last_tick = now;

  // advance each channel
  for (int i = 0; i < 3; i++) {
    Channel &c = ch[i];
    if (c.ticks_left > 0) {
      c.ticks_left--;
      if (c.ticks_left > 0) continue;
    }
    // time for next command
    processCommand(c);
  }
}
