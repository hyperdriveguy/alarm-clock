// include/TestSong4.h

#ifndef TEST_SONG4_H
#define TEST_SONG4_H

// ─── Channel 1 ───  (bytes:  14) ──────────────────────
// ticks: (4×4) + (4×4) = 32
static const uint8_t Ch1[] = {
  0xDA,0x00,0x64,     // tempo=100
  0xD4,               // octave=3
  0x14,0x24,0x34,0x44, // four notes len=4→16 ticks
  0xDA,0x00,0x50,     // tempo=80
  0xD3,               // octave=4
  0x54,0x64,0x74,0x84, // four notes len=4→16 ticks
  0xFF
};

// ─── Channel 2 ───  (bytes: 13) ──────────────────────
// ticks: (2×8)+(8)+(2×8) = 32
static const uint8_t Ch2[] = {
  0xD5,               // octave=2
  0xC1,0x00,0x80,0x00, // sound→8 ticks
  0xC3,0x00,0x80,0x00, // sound→8 ticks
  0x08,               // rest len=8
  0xD3,               // octave=4
  0xC5,0x00,0x80,0x00, // sound→8 ticks
  0xC7,0x00,0x80,0x00, // sound→8 ticks
  0xFF
};

// ─── Channel 3 ───  (bytes: 10) ──────────────────────
// ticks: 8×4 = 32
static const uint8_t Ch3[] = {
  0xD3,               // octave=4
  0x14,0x24,0x34,0x44, // notes len=4 (×4=16)
  0x54,0x64,0x74,0x84, // notes len=4 (×4=16)
  0xFF
};

#endif  // TEST_SONG4_H
