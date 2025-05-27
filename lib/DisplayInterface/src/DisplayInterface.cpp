#include "DisplayInterface.h"
#include <Wire.h>

DisplayInterface::DisplayInterface(uint8_t i2c_addr, uint8_t cols, uint8_t rows,
                                   uint8_t sda, uint8_t scl, uint8_t backlight_pin)
    : lcd(i2c_addr, cols, rows),
      backlightPin(backlight_pin),
      sdaPin(sda),             // store custom SDA pin
      sclPin(scl) {            // store custom SCL pin
}

void DisplayInterface::begin() {
  Wire.begin(sdaPin, sclPin); // Initialize I2C with custom SDA/SCL pins
  lcd.init();
  lcd.backlight();
  const uint8_t backlightPWMChannel = 3;
  ledcAttachPin(backlightPin, backlightPWMChannel);
  ledcSetup(backlightPWMChannel, 5000, 8);
  ledcWrite(backlightPWMChannel, 255);
}

void DisplayInterface::setBrightness(uint8_t level) {
  ledcWrite(3, level);
}

void DisplayInterface::showTime(const String& timeStr) {
  lcd.setCursor(0, 0);
  lcd.print("Time: " + timeStr);
}

void DisplayInterface::showDate(const String& dateStr) {
  lcd.setCursor(0, 1);
  lcd.print("Date: " + dateStr);
}

void DisplayInterface::showMessage(const String& line1, const String& line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
