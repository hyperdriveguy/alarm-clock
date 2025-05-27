#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#include <LiquidCrystal_I2C.h>

class DisplayInterface {
 public:
  DisplayInterface(uint8_t i2c_addr = 0x27, uint8_t cols = 12, uint8_t rows = 2,
                   uint8_t sda = 15, uint8_t scl = 16, uint8_t backlight_pin = 17);

  void begin();
  void setBrightness(uint8_t level);  // 0–255
  void showTime(const String& timeStr);
  void showDate(const String& dateStr);
  void showMessage(const String& line1, const String& line2 = "");

 private:
  LiquidCrystal_I2C lcd;
  uint8_t backlightPin;
  uint8_t sdaPin;  // added: store custom SDA pin
  uint8_t sclPin;  // added: store custom SCL pin
};

#endif  // DISPLAY_INTERFACE_H
