// include/TestSong.h

#ifndef TEST_SONG_H
#define TEST_SONG_H

// Tempo = 100 ms (0x0064)
static const uint8_t Ch1[] = {
  0xDA, 0x00, 0x64,   // tempo 100ms
  0x14,               // C_ length=4
  0x24,               // D_ length=4
  0x34,               // E_ length=4
  0x44,               // F_ length=4
  0xFF                // endchannel (loop)
};

static const uint8_t Ch2[] = {
  0xDA, 0x00, 0x64,   // same tempo
  0x14,               // C_ length=4
  0x24,               // D_ length=4
  0x34,               // E_ length=4
  0x44,               // F_ length=4
  0xFF
};

static const uint8_t Ch3[] = {
  0xDA, 0x00, 0x64,
  0x14,               // C_
  0x24,               // D_
  0x34,               // E_
  0x44,               // F_
  0xFF
};

#endif  // TEST_SONG_H
