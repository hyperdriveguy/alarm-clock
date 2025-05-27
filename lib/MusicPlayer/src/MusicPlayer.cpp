// lib/MusicPlayer/src/MusicPlayer.cpp

#include "MusicPlayer.h"

const uint16_t MusicPlayer::FREQ_REG[25] = {
    0x0000,
    0xF82C, 0xF89D, 0xF907, 0xF96B, 0xF9CA, 0xFA23, 0xFA77, 0xFAC7,
    0xFB12, 0xFB58, 0xFB9B, 0xFBDA,
    0xFC16, 0xFC4E, 0xFC83, 0xFCB5, 0xFCE5, 0xFD11, 0xFD3B, 0xFD63,
    0xFD89, 0xFDAC, 0xFDCD, 0xFDED
};

MusicPlayer::MusicPlayer(const int buzzer_pins[3], const int ledc_channels[3], 
                         int ledc_timer, int ledc_res_bits)
    : ledc_timer_(ledc_timer), ledc_res_bits_(ledc_res_bits),
      tick_ms_(100), last_tick_(0), tick_count_(0), playing_(false) {
    
    for (int i = 0; i < 3; i++) {
        buzzer_pins_[i] = buzzer_pins[i];
        ledc_channels_[i] = ledc_channels[i];
    }
}

void MusicPlayer::begin() {
    initCommandSizes();
    
    for (int i = 0; i < 3; i++) {
        ledcSetup(ledc_channels_[i], 1000, ledc_res_bits_);
        ledcAttachPin(buzzer_pins_[i], ledc_channels_[i]);
        silenceChannel(i);
    }
}

void MusicPlayer::play(const uint8_t* ch1_data, const uint8_t* ch2_data, const uint8_t* ch3_data) {
    const uint8_t* channel_data[3] = { ch1_data, ch2_data, ch3_data };
    
    for (int i = 0; i < 3; i++) {
        channels_[i] = { channel_data[i], 0, 4, 128, 0 };
        processCommand(i);
    }
    
    playing_ = true;
    last_tick_ = millis();
    tick_count_ = 0;
    MDEBUG_PRINTLN("MusicPlayer: Started playing");
}

void MusicPlayer::stop() {
    playing_ = false;
    
    for (int i = 0; i < 3; i++) {
        silenceChannel(i);
    }
    
    MDEBUG_PRINTLN("MusicPlayer: Stopped");
}

bool MusicPlayer::isPlaying() const {
    return playing_;
}

void MusicPlayer::update() {
    if (!playing_) return;
    
    unsigned long now = millis();
    if (now - last_tick_ < tick_ms_) return;
    
    last_tick_ = now;
    tick_count_++;
    MDEBUG_PRINTF("MusicPlayer: Tick #%lu @%lums\n", tick_count_, now);
    
    for (int i = 0; i < 3; i++) {
        auto& ch = channels_[i];
        MDEBUG_PRINTF("PreCh%d tl=%d idx=%u\n", i+1, ch.ticks_left, ch.idx);
        
        if (ch.ticks_left > 0) {
            ch.ticks_left--;
            MDEBUG_PRINTF(" dec→%d\n", ch.ticks_left);
            
            if (ch.ticks_left == 0) {
                MDEBUG_PRINTF("Ch%d REST end: silencing\n", i+1);
                silenceChannel(i);
            }
            
            if (ch.ticks_left > 0) continue;
        }
        
        processCommand(i);
    }
}

void MusicPlayer::initCommandSizes() {
    for (int i = 0; i < 256; i++) command_sizes_[i] = 1;
    for (int c = 0xC0; c <= 0xCF; c++) command_sizes_[c] = 4;
    command_sizes_[0xD2] = 5;
    for (int c = 0xD0; c <= 0xD7; c++) command_sizes_[c] = 1;
    command_sizes_[0xD8] = 2; command_sizes_[0xD9] = 2;
    command_sizes_[0xDA] = 3; command_sizes_[0xDB] = 2;
    command_sizes_[0xDC] = 2; command_sizes_[0xDD] = 2;
    command_sizes_[0xDE] = 2; command_sizes_[0xDF] = 1;
    command_sizes_[0xE0] = 3; command_sizes_[0xE1] = 3;
    command_sizes_[0xE2] = 2; command_sizes_[0xE3] = 2;
    command_sizes_[0xE4] = 2; command_sizes_[0xE5] = 2;
    command_sizes_[0xE6] = 2; command_sizes_[0xEB] = 2;
    command_sizes_[0xEC] = 1; command_sizes_[0xED] = 1;
    command_sizes_[0xEE] = 3; command_sizes_[0xEF] = 2;
    command_sizes_[0xF0] = 2;
    for (int c = 0xF1; c <= 0xF9; c++) command_sizes_[c] = 1;
    command_sizes_[0xFF] = 1; command_sizes_[0xFC] = 3;
    command_sizes_[0xFD] = 4; command_sizes_[0xFE] = 3;
    command_sizes_[0xFB] = 4; command_sizes_[0xEA] = 3;
}

float MusicPlayer::getFreq(uint8_t pitch, uint8_t oct) {
    if (pitch == 0 || pitch > 12) return 0;
    uint8_t baseOct = oct < 2 ? 0 : 1;
    uint16_t reg = FREQ_REG[pitch + baseOct * 12] & 0x07FF;
    float f = 131072.0f / float(2048 - reg);
    return f * powf(2.0f, float(oct - baseOct));
}

void MusicPlayer::processCommand(int i) {
    auto& ch = channels_[i];
    
    // Handle loop commands first to avoid advancing idx
    if (ch.data[ch.idx] == 0xFF || ch.data[ch.idx] == 0xEA) {
        ch.idx = 0;
        MDEBUG_PRINTF("Ch%d LOOP\n", i+1);
    }
    
    uint8_t cmd = ch.data[ch.idx++];
    MDEBUG_PRINTF("Ch%d cmd=0x%02X idx=%u ", i+1, cmd, ch.idx-1);

    if (cmd <= 0xCF) {
        uint8_t p = cmd >> 4, L = cmd & 0xF;
        ch.ticks_left = (L == 0) ? 16 : L;
        float f = getFreq(p, ch.octave);
        
        if (p == 0) {
            MDEBUG_PRINTF("REST len=%u\n", L);
            silenceChannel(i);
        } else {
            MDEBUG_PRINTF("NOTE p=%u len=%u f=%.1fHz\n", p, L, f);
            ledcWriteTone(ledc_channels_[i], uint32_t(f + 0.5f));
            ledcWrite(ledc_channels_[i], ch.volume);
        }
        return;
    }
    
    if ((cmd & 0xF8) == 0xD0) {
        ch.octave = 0xD7 - cmd;
        MDEBUG_PRINTF("OCT→%u\n", ch.octave);
        processCommand(i);
        return;
    }
    
    if (cmd == 0xDA) {
        uint16_t t = (uint16_t(ch.data[ch.idx]) << 8) | ch.data[ch.idx + 1];
        ch.idx += 2;
        tick_ms_ = t;
        last_tick_ = millis();
        MDEBUG_PRINTF("TEMPO→%ums\n", t);
        processCommand(i);
        return;
    }
    
    if (cmd == 0xDC) {
        ch.volume = ch.data[ch.idx++];
        MDEBUG_PRINTF("VOL→%u\n", ch.volume);
        processCommand(i);
        return;
    }
    
    if (cmd == 0xFF || cmd == 0xEA) {
        processCommand(i);
        return;
    }
    
    uint8_t s = command_sizes_[cmd];
    MDEBUG_PRINTF("SKIP %u\n", s - 1);
    if (s > 1) ch.idx += s - 1;
    processCommand(i);
}

void MusicPlayer::silenceChannel(int channel) {
    ledcWriteTone(ledc_channels_[channel], 0);
    ledcWrite(ledc_channels_[channel], 0);
}