// include/TestSong6.h

#ifndef TEST_SONG6_H
#define TEST_SONG6_H

// ─── Channel 1 (9 bytes) ───────────────────────────────────────────────
// ticks = 4 × 8 = 32
static const uint8_t Ch1[] = {
  0xDA, 0x00, 0x64,   // tempo = 100 ms
  0xD4,               // octave → 3
  0x18, 0x18, 0x18, 0x18, // four notes, len=8 ticks each = 32 ticks
  0xFF                // loop
};

// ─── Channel 2 (5 bytes) ───────────────────────────────────────────────
// ticks = 15 + 15 + 2 = 32
static const uint8_t Ch2[] = {
  0xD4,     // octave → 3
  0x1F,     // note/rest with pitch=1, len=15
  0x1F,     // pitch=1, len=15
  0x02,     // rest, len=2
  0xFF      // loop
};

// ─── Channel 3 (6 bytes) ───────────────────────────────────────────────
// ticks = 4 × 8 = 32
static const uint8_t Ch3[] = {
  0xD4,               // octave → 3
  0x18, 0x18, 0x18, 0x18, // four notes len=8
  0xFF                // loop
};

#endif  // TEST_SONG6_H
