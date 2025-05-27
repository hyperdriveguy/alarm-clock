// include/TestSong5.h

#ifndef TEST_SONG5_H
#define TEST_SONG5_H

// ─── Channel 1 (bytes: 13) ────────────────────────────────
// 0xDA,0x00,0x64      ; tempo 100ms
// 0xD3                ; octave = 4
// 0x18,0x28          ; C_,D_ len=8+8 =16 ticks
// 0xDA,0x00,0x50      ; tempo 80ms
// 0xD4                ; octave = 3
// 0x38,0x48          ; E_,F_ len=8+8 =16 ticks → total 32 ticks
// 0xFF                ; loop
static const uint8_t Ch1[] = {
  0xDA, 0x00, 0x64,
  0xD3,
  0x18, 0x28,
  0xDA, 0x00, 0x50,
  0xD4,
  0x38, 0x48,
  0xFF
};

// ─── Channel 2 (bytes:  6) ────────────────────────────────
// 0xD4                ; octave = 3
// 0x58,0x68,0x78,0x88  ; G_,A_,B_,C_ len=8×4=32 ticks
// 0xFF                ; loop
static const uint8_t Ch2[] = {
  0xD4,
  0x58, 0x68, 0x78, 0x88,
  0xFF
};

// ─── Channel 3 (bytes:  8) ────────────────────────────────
// 0xD5                ; octave = 2
// 0x54                ; E_ len=4 ticks
// 0x6C                ; F# len=12 ticks
// 0x0C                ; rest len=12 ticks (0x0C = 12)
// 0x24                ; D_ len=4 ticks → 4+12+12+4 = 32 ticks
// 0xFF                ; loop
static const uint8_t Ch3[] = {
  0xD5,
  0x54,
  0x6C,
  0x0C,
  0x24,
  0xFF
};

#endif  // TEST_SONG5_H
