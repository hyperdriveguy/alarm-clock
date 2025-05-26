// include/SoundEngine.h

#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <Arduino.h>
#include "somebody.h"   // extern const uint8_t Ch1[], Ch2[], Ch3[]

//––– hardware config ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
static constexpr int BUZZER_PINS[3]   = { 40, 41, 42 };
static constexpr int LEDC_CHANNEL[3]  = { 0, 1, 2 };
static constexpr int LEDC_TIMER       = 0;
static constexpr int LEDC_RES_BITS    = 8;

//––––– Game Boy frequency register table (two octaves) –––––––––––––––––––
// dw 0     ; rest
// dw $F82C ; C_, $F89D ; C#, … $FBDA ; B_  (octave 0)
// dw $FC16 ; C_, … $FDED ; B_  (octave 1)
static const uint16_t FREQ_REG[25] = {
  0x0000,
  0xF82C, 0xF89D, 0xF907, 0xF96B, 0xF9CA, 0xFA23, 0xFA77, 0xFAC7,
  0xFB12, 0xFB58, 0xFB9B, 0xFBDA,
  0xFC16, 0xFC4E, 0xFC83, 0xFCB5, 0xFCE5, 0xFD11, 0xFD3B, 0xFD63,
  0xFD89, 0xFDAC, 0xFDCD, 0xFDED
};

//––– per‑channel parser state –––––––––––––––––––––––––––––––––––––––––––––
struct Channel {
  const uint8_t* data;
  size_t         idx;
  uint8_t        octave;      // 0–7
  uint8_t        volume;      // 0–255
  int            ticks_left;  // ticks remaining for current note
  int            ledc_ch;
};

// globals defined in main.cpp:
extern Channel      ch[3];
extern uint8_t      COMMAND_SIZES[256];
extern unsigned long tick_ms;

/**
 * @brief  Populate COMMAND_SIZES[] from your Python COMMAND_SIZES map.
 */
static inline void initCommandSizes() {
  for (int i = 0; i < 256; i++) {
    COMMAND_SIZES[i] = 1;  // default size = 1
  }
  // sound commands 0xC0–0xCF = 4 bytes
  for (int c = 0xC0; c <= 0xCF; c++) {
    COMMAND_SIZES[c] = 4;
  }
  COMMAND_SIZES[0xD2] = 5;   // noise
  for (int c = 0xD0; c <= 0xD7; c++) {
    COMMAND_SIZES[c] = 1;    // octave
  }
  COMMAND_SIZES[0xD8] = 2;   // notetype
  COMMAND_SIZES[0xD9] = 2;   // pitchoffset
  COMMAND_SIZES[0xDA] = 3;   // tempo
  COMMAND_SIZES[0xDB] = 2;   // dutycycle
  COMMAND_SIZES[0xDC] = 2;   // volume/intensity
  COMMAND_SIZES[0xDD] = 2;   // soundinput
  COMMAND_SIZES[0xDE] = 2;   // sound_duty
  COMMAND_SIZES[0xDF] = 1;   // togglesfx
  COMMAND_SIZES[0xE0] = 3;   // slidepitchto
  COMMAND_SIZES[0xE1] = 3;   // vibrato
  COMMAND_SIZES[0xE2] = 2;   // unknown0xE2
  COMMAND_SIZES[0xE3] = 2;   // togglenoise
  COMMAND_SIZES[0xE4] = 2;   // panning
  COMMAND_SIZES[0xE5] = 2;   // volume
  COMMAND_SIZES[0xE6] = 2;   // tone
  COMMAND_SIZES[0xEB] = 2;   // newsong
  COMMAND_SIZES[0xEC] = 1;   // sfxpriorityon
  COMMAND_SIZES[0xED] = 1;   // sfxpriorityoff
  COMMAND_SIZES[0xEE] = 3;   // unknown0xEE
  COMMAND_SIZES[0xEF] = 2;   // stereopanning
  COMMAND_SIZES[0xF0] = 2;   // sfxtogglenoise
  for (int c = 0xF1; c <= 0xF9; c++) {
    COMMAND_SIZES[c] = 1;    // music0xF1–0xF9
  }
  // control flow
  COMMAND_SIZES[0xFF] = 1;   // endchannel
  COMMAND_SIZES[0xFC] = 3;   // jumpchannel
  COMMAND_SIZES[0xFD] = 4;   // loopchannel
  COMMAND_SIZES[0xFE] = 3;   // callchannel
  COMMAND_SIZES[0xFB] = 4;   // jumpif
  COMMAND_SIZES[0xEA] = 3;   // restartchannel
}

/**
 * @brief  Compute the output frequency for a given pitch (0=rest, 1=C_, … 12=B_)
 *         and octave (0..7) using the GB hardware formula.
 * @param  pitch   high nibble from the note command
 * @param  octave  current octave setting
 * @return frequency in Hz (0 for rest)
 */
static inline float getFrequency(uint8_t pitch, uint8_t octave) {
  if (pitch == 0 || pitch > 12) return 0.0f;
  // use one of the two built‑in octaves, then scale
  uint8_t baseOct = (octave < 2 ? 0 : 1);
  uint16_t reg    = FREQ_REG[pitch + baseOct * 12];
  uint16_t N      = reg & 0x07FF;
  float    f      = 131072.0f / float(2048 - N);
  // scale by 2^(octave - baseOct)
  return f * powf(2.0f, float(octave - baseOct));
}

/**
 * @brief  Read & execute the next command for channel @p c.
 *         Only **note** sets c.ticks_left; everything else
 *         executes instantly (no tick consumed).
 */
static inline void processCommand(Channel &c) {
  uint8_t cmd = c.data[c.idx++];
  if (cmd <= 0xCF) {
    // note: upper nibble = pitch, lower nibble = length
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
  }
  else if ((cmd & 0xF8) == 0xD0) {
    // octave command
    c.octave = 0xD7 - cmd;
  }
  else if (cmd == 0xDA) {
    // tempo MSB/LSB
    uint16_t t = (uint16_t(c.data[c.idx]) << 8) | c.data[c.idx + 1];
    c.idx += 2;
    tick_ms = t;
  }
  else if (cmd == 0xDC) {
    // volume/intensity
    c.volume = c.data[c.idx++];
  }
  else {
    // skip any other command’s parameters
    uint8_t sz = COMMAND_SIZES[cmd];
    if (sz > 1) c.idx += (sz - 1);
  }
}

#endif  // SOUND_ENGINE_H
