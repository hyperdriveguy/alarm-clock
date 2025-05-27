// include/TestSong3.h

#ifndef TEST_SONG3_H
#define TEST_SONG3_H

// ─── Channel 1: short commands + two tempo changes ───────────────────────
// total ticks = 32
static const uint8_t Ch1[] = {
  // set base tempo 100 ms
  0xDA, 0x00, 0x64,
  // octave 3
  0xD4,
  // four notes, each len=4 ticks (4×4=16)
  0x14, 0x24, 0x34, 0x44,
  // speed up to 80 ms
  0xDA, 0x00, 0x50,
  // octave 4
  0xD3,
  // four notes, len=4 (adds 16 ticks) → total 32 ticks
  0x54, 0x64, 0x74, 0x84,
  // loop
  0xFF
};

// ─── Channel 2: no tempo commands, longer encodings ─────────────────────
// total ticks = 32
static const uint8_t Ch2[] = {
  // octave 2
  0xD5,
  // two big “sound” commands (4 bytes each) that we’ll treat as “notes” of len=8 ticks
  // (we don’t actually use their frequency payloads, but we skip 4 bytes)
  0xC1, 0x00, 0x80, 0x00,   // sound (pitch=1) → 8 ticks
  0xC3, 0x00, 0x80, 0x00,   // sound (pitch=3) → 8 ticks
  // rest: 8 ticks
  0x10 | 0x0,               // rest, len=8→ lower nibble=8
  // octave 3
  0xD3,
  // two more big sounds → 8 + 8 = 16 ticks
  0xC5, 0x00, 0x80, 0x00,
  0xC7, 0x00, 0x80, 0x00,
  // loop
  0xFF
};

// ─── Channel 3: mixed note and octave, no tempo ─────────────────────────
// total ticks = 32
static const uint8_t Ch3[] = {
  // octave 4
  0xD3,
  // eight quarter‑length notes (len=4 ticks each → 8×4 = 32)
  0x14, 0x24, 0x34, 0x44,
  0x54, 0x64, 0x74, 0x84,
  // loop
  0xFF
};

#endif  // TEST_SONG3_H
