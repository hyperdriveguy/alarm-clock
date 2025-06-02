#ifndef MENU_INTERFACE_H
#define MENU_INTERFACE_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "DisplayInterface.h"

// Forward‐declare Adafruit types (already included by DisplayInterface.h)
class Adafruit_ILI9341;
class Adafruit_FT6206;

/**
 * @class MenuInterface
 * @brief Renders a touchscreen‐driven settings menu (WiFi, Brightness, About, etc.) on a TFT.
 *
 * This class uses DisplayInterface (Adafruit_ILI9341 + FT6206) to draw buttons,
 * handle touches, present an on‐screen keyboard for text entry, and store WiFi
 * credentials.  
 *
 * Usage:
 *   DisplayInterface display(&tft, &touch);
 *   MenuInterface menu(&display);
 *   void setup() {
 *     display.begin(40000000);
 *     menu.begin();
 *   }
 *   void loop() {
 *     menu.handle();  // call inside your main loop
 *   }
 */
class MenuInterface {
 public:
  /**
   * @brief Construct a MenuInterface.
   * @param display Pointer to an initialized DisplayInterface instance.
   */
  MenuInterface(DisplayInterface* display);

  /**
   * @brief Call once in setup(): loads stored WiFi creds (if any), draws main menu.
   */
  void begin();

  /**
   * @brief Call inside loop(): checks for touches, updates current menu screen.
   */
  void handle();

  /**
   * @brief Optionally expose activate if needed elsewhere.
   */
  void activate();

 private:
  DisplayInterface* disp;
  Preferences prefs;

  // === Stored settings ===
  String wifi_ssid;
  String wifi_password;
  uint8_t brightness;  // 0–255

  // === Menu state ===
  enum class Screen {
    MainMenu,
    WiFiMenu,
    EnterSSID,
    EnterPassword,
    Connecting,
    BrightnessMenu,
    AboutScreen
  };
  Screen currentScreen;

  // For storing partially-entered text during on‐screen keyboard input
  String tempInput;

  // NEW: Flag indicating whether the settings menu is active.
  bool settingsActive;

  // === Helpers for drawing ===
  struct Button {
    int16_t x, y, w, h;
    const char* label;
  };

  void drawMainMenu();
  void drawWiFiMenu();
  void drawBrightnessMenu();
  void drawAboutScreen();

  // === Touch‐handling routines for each screen ===
  void handleMainMenuTouch(uint16_t x, uint16_t y);
  void handleWiFiMenuTouch(uint16_t x, uint16_t y);
  void handleBrightnessMenuTouch(uint16_t x, uint16_t y);
  void handleAboutScreenTouch(uint16_t x, uint16_t y);

  // === On‐screen keyboard for SSID/password ===
  String showKeyboard(const String& prompt, const String& initial = "");

  // Attempts to connect; returns true if successful (or false and error message shown)
  bool attemptWiFiConnect(const String& ssid, const String& pass);

  // === Utility ===
  void clearTouchBuffer();  // wait until no touch is detected
};

#endif  // MENU_INTERFACE_H
