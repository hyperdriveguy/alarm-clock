// lib/MusicPlayer/include/MusicPlayer.h

#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <Arduino.h>
#include "MusicTimer.h"
#include "DebugLog.h"

class MusicPlayer {
public:
    /**
     * Constructor
     * @param buzzer_pins Array of 3 pin numbers for buzzers
     * @param ledc_channels Array of 3 LEDC channel numbers
     * @param ledc_timer LEDC timer number
     * @param ledc_res_bits LEDC resolution bits
     */
    MusicPlayer(const int buzzer_pins[3], const int ledc_channels[3], 
                int ledc_timer = 0, int ledc_res_bits = 8);
    
    /**
     * Initialize the music player
     */
    void begin();
    
    /**
     * Start playing a song
     * @param ch1_data Channel 1 music data
     * @param ch2_data Channel 2 music data  
     * @param ch3_data Channel 3 music data
     */
    void play(const uint8_t* ch1_data, const uint8_t* ch2_data, const uint8_t* ch3_data);
    
    /**
     * Stop playing and silence all channels
     */
    void stop();
    
    /**
     * Check if currently playing
     * @return true if playing, false if stopped
     */
    bool isPlaying() const;
    
    /**
     * Update music playback - call this in your main loop
     */
    void update();
    
private:
    struct Channel {
        const uint8_t* data;
        size_t idx;
        uint8_t octave;
        uint8_t volume;
        int ticks_left;
    };

    // Global timer
    MusicTimer _timer;
    
    // Hardware configuration
    int buzzer_pins_[3];
    int ledc_channels_[3];
    int ledc_timer_;
    int ledc_res_bits_;
    
    // Player state
    Channel channels_[3];
    uint8_t command_sizes_[256];
    unsigned long tick_ms_;
    unsigned long last_tick_;
    unsigned long tick_count_;
    bool playing_;
    
    // GB frequency table
    static const uint16_t FREQ_REG[25];
    
    void initCommandSizes();
    float getFreq(uint8_t pitch, uint8_t oct);
    void processCommand(int channel);
    void silenceChannel(int channel);
};

#endif // MUSIC_PLAYER_H