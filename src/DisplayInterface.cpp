#include "DisplayInterface.h"

DisplayInterface::DisplayInterface(Adafruit_ILI9341* tft_display, Adafruit_FT6206* touch_controller,
                                   uint8_t backlight_pin)
    : tft(tft_display),
      touchController(touch_controller),
      backlightPin(backlight_pin),
      backlightChannel(7),  // Use channel 7 for backlight PWM
      prevTimeValid(false) {
}

void DisplayInterface::begin(uint32_t freq) {
    // Initialize TFT display
    tft->begin(freq);
    tft->setRotation(3);  // Landscape orientation (320x240)
    tft->invertDisplay(true);
    tft->fillScreen(BLACK);
    
    // Initialize touch controller
    if (!touchController->begin(40)) {  // Pass in sensitivity coefficient
        Serial.println("Touch controller initialization failed!");
    }
    
    // Setup backlight PWM
    ledcSetup(backlightChannel, 5000, 8);  // 5kHz frequency, 8-bit resolution
    ledcAttachPin(backlightPin, backlightChannel);
    ledcWrite(backlightChannel, 255);  // Full brightness initially
    
    // Display startup message
    tft->setTextColor(WHITE);
    tft->setTextSize(2);
    drawCenteredText("Display Ready", SCREEN_HEIGHT/2);
}

void DisplayInterface::setBrightness(uint8_t level) {
    ledcWrite(backlightChannel, level);
}

void DisplayInterface::showTime(const ClockDateTime* time_struct) {
    if (!prevTimeValid) {
        clearScreen();
        // Draw full time (HH:MM:SS)
        tft->setTextSize(4);
        tft->setTextColor(CYAN);
        String timeStr = String(time_struct->hour < 10 ? "0" : "") + String(time_struct->hour) + ":" +
                         String(time_struct->minute < 10 ? "0" : "") + String(time_struct->minute) + ":" +
                         String(time_struct->second < 10 ? "0" : "") + String(time_struct->second);
        drawCenteredText(timeStr, TIME_Y, CYAN, 4);
        
        // Draw full date (YYYY-MM-DD)
        tft->setTextSize(2);
        tft->setTextColor(YELLOW);
        String dateStr = String(time_struct->year) + "-" +
                         String(time_struct->month < 10 ? "0" : "") + String(time_struct->month) + "-" +
                         String(time_struct->day < 10 ? "0" : "") + String(time_struct->day);
        drawCenteredText(dateStr, DATE_Y, YELLOW, 2);
        
        // Save the time for later comparison.
        prev_time_struct = *time_struct;
        prevTimeValid = true;
        return;
    }
    
    // Update only if any time component changed
    updateTimeDisplay(time_struct, &prev_time_struct);
    
    // Copy new time into the internal record.
    prev_time_struct = *time_struct;
}

void DisplayInterface::updateTimeDisplay(const ClockDateTime* current, const ClockDateTime* previous) {
    tft->setTextSize(4);
    
    // Update time if any component changed
    if (messageJustShown || current->hour != previous->hour || current->minute != previous->minute || current->second != previous->second) {
        // Clear the time area
        tft->fillRect(0, TIME_Y - 20, SCREEN_WIDTH, 50, BLACK);
        
        // Redraw time
        tft->setTextColor(CYAN);
        String timeStr = String(current->hour < 10 ? "0" : "") + String(current->hour) + ":" +
                        String(current->minute < 10 ? "0" : "") + String(current->minute) + ":" +
                        String(current->second < 10 ? "0" : "") + String(current->second);
        drawCenteredText(timeStr, TIME_Y, CYAN, 4);
    }
    
    // Update date if any component changed
    if (messageJustShown || current->year != previous->year || current->month != previous->month || current->day != previous->day) {
        // Clear the date area
        tft->fillRect(0, DATE_Y - 10, SCREEN_WIDTH, 25, BLACK);
        
        // Redraw date
        tft->setTextSize(2);
        tft->setTextColor(YELLOW);
        String dateStr = String(current->year) + "-" +
                        String(current->month < 10 ? "0" : "") + String(current->month) + "-" +
                        String(current->day < 10 ? "0" : "") + String(current->day);
        drawCenteredText(dateStr, DATE_Y, YELLOW, 2);
    }
}

void DisplayInterface::showMessage(const String& line1, const String& line2) {
    messageJustShown = true;
    clearScreen();
    
    // Show first line
    tft->setTextSize(3);
    tft->setTextColor(WHITE);
    drawCenteredText(line1, MESSAGE_LINE1_Y, WHITE, 3);
    
    // Show second line if provided
    if (line2.length() > 0) {
        tft->setTextSize(2);
        tft->setTextColor(GRAY);
        drawCenteredText(line2, MESSAGE_LINE2_Y, GRAY, 2);
    }
}

bool DisplayInterface::isTouched() {
    return touchController->touched();
}

void DisplayInterface::getTouchPoint(uint16_t* x, uint16_t* y) {
    TS_Point p = touchController->getPoint();
    
    // Map touch coordinates to screen coordinates
    // The touch controller may need calibration depending on orientation
    *x = map(p.x, 0, 320, 0, SCREEN_WIDTH);
    *y = map(p.y, 0, 240, 0, SCREEN_HEIGHT);
}

void DisplayInterface::clearScreen() {
    tft->fillScreen(BLACK);
}

void DisplayInterface::drawCenteredText(const String& text, int16_t y, uint16_t color, uint8_t textSize) {
    tft->setTextSize(textSize);
    tft->setTextColor(color);
    
    // Calculate text bounds
    int16_t x1, y1;
    uint16_t w, h;
    tft->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    
    // Center horizontally
    int16_t x = (SCREEN_WIDTH - w) / 2;
    
    tft->setCursor(x, y);
    tft->print(text);
}