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

void DisplayInterface::showTime(const ClockDateTime* time_struct) {
  // Initial full draw
  if (prev_time_struct == nullptr) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("%02d:%02d:%02d",
               time_struct->hour,
               time_struct->minute,
               time_struct->second);
    lcd.setCursor(0, 1);
    lcd.printf("%04d-%02d-%02d",
               time_struct->year,
               time_struct->month,
               time_struct->day);
    prev_time_struct = time_struct;  // take ownership
    return;
  }

  // Update only changed fields
  if (time_struct->hour   != prev_time_struct->hour)   { lcd.setCursor(0, 0); lcd.printf("%02d", time_struct->hour); }
  if (time_struct->minute != prev_time_struct->minute) { lcd.setCursor(3, 0); lcd.printf("%02d", time_struct->minute); }
  if (time_struct->second != prev_time_struct->second) { lcd.setCursor(6, 0); lcd.printf("%02d", time_struct->second); }

  if (time_struct->year  != prev_time_struct->year)  { lcd.setCursor(0, 1); lcd.printf("%04d", time_struct->year); }
  if (time_struct->month != prev_time_struct->month) { lcd.setCursor(5, 1); lcd.printf("%02d", time_struct->month); }
  if (time_struct->day   != prev_time_struct->day)   { lcd.setCursor(8, 1); lcd.printf("%02d", time_struct->day); }

  // Swap pointers and delete old buffer
  delete prev_time_struct;
  prev_time_struct = time_struct;
}

void DisplayInterface::showMessage(const String& line1, const String& line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
