// include/TestSong.h

#ifndef TEST_SONG_H
#define TEST_SONG_H

// ––– A test pattern for all three channels ––––––––––––––––––––––––––––––––
// 1) Start @100 ms ticks, play C, D, E, F (len=4)
// 2) Slight tempo increase to 80 ms + octave up
// 3) Play G, A (len=4)
// 4) Major tempo slow‑down to 200 ms + octave down
// 5) Play C (len=2), D (len=6), E (len=4)
// 6) Loop forever

static const uint8_t Ch1[] = {
  // 1) initial tempo & notes
  0xDA, 0x00, 0x64,   // tempo = 100 ms
  0x14, 0x24, 0x34, 0x44,  // C_4, D_4, E_4, F_4

  // 2) slight speed‑up + octave up
  0xDA, 0x00, 0x50,   // tempo = 80 ms
  0xD3,               // octave → 0xD7–0xD3 = 4 (one up)
  0x84, 0xA4,         // G_4, A_4

  // 4) major slow‑down + octave down
  0xDA, 0x00, 0xC8,   // tempo = 200 ms
  0xD6,               // octave → 0xD7–0xD6 = 1 (down)
  0x12, 0x26, 0x34,   // C_2, D_6, E_4 (varying lengths)

  0xFF                // loop
};

static const uint8_t Ch2[] = {
  // identical to Ch1
  0xDA, 0x00, 0x64,
  0x14, 0x24, 0x34, 0x44,
  0xDA, 0x00, 0x50,
  0xD3,
  0x84, 0xA4,
  0xDA, 0x00, 0xC8,
  0xD6,
  0x12, 0x26, 0x34,
  0xFF
};

static const uint8_t Ch3[] = {
  // identical to Ch1
  0xDA, 0x00, 0x64,
  0x14, 0x24, 0x34, 0x44,
  0xDA, 0x00, 0x50,
  0xD3,
  0x84, 0xA4,
  0xDA, 0x00, 0xC8,
  0xD6,
  0x12, 0x26, 0x34,
  0xFF
};

#endif  // TEST_SONG_H
