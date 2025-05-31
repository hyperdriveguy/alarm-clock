#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <ntp_clock64.h>

// Color definitions for 16-bit color (RGB565)
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF
#define ORANGE   0xFD20
#define GRAY     0x8410
#define DARKGRAY 0x4208

class DisplayInterface {
 public:
  DisplayInterface(Adafruit_ILI9341* tft_display, Adafruit_FT6206* touch_controller, 
                   uint8_t backlight_pin = 9);

  void begin();
  void setBrightness(uint8_t level);  // 0–255

  /**
   * Takes ownership of `time_struct`, which must be allocated with `new`.
   * It will be deleted internally after the next update.
   */
  void showTime(const ClockDateTime* time_struct);
  
  void showMessage(const String& line1, const String& line2 = "");
  
  // Touch functionality
  bool isTouched();
  void getTouchPoint(uint16_t* x, uint16_t* y);
  
  // Display control
  void clearScreen();
  void drawCenteredText(const String& text, int16_t y, uint16_t color = WHITE, uint8_t textSize = 2);

 private:
  Adafruit_ILI9341* tft;
  Adafruit_FT6206* touchController;
  uint8_t backlightPin;
  uint8_t backlightChannel;
  const ClockDateTime* prev_time_struct;
  
  // Display dimensions
  static const uint16_t SCREEN_WIDTH = 320;
  static const uint16_t SCREEN_HEIGHT = 240;
  
  // Text positioning for time display
  static const int16_t TIME_Y = 80;
  static const int16_t DATE_Y = 140;
  static const int16_t MESSAGE_LINE1_Y = 80;
  static const int16_t MESSAGE_LINE2_Y = 140;
  
  void drawTimeField(const String& text, int16_t x, int16_t y, uint16_t color = WHITE);
  void updateTimeDisplay(const ClockDateTime* current, const ClockDateTime* previous);
};

#endif  // DISPLAY_INTERFACE_H