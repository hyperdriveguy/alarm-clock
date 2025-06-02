#include "MenuInterface.h"

// Adafruit GFX / ILI9341 are already included by DisplayInterface.h
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>

/// ----- Constructor & begin() -----

MenuInterface::MenuInterface(DisplayInterface* display)
    : disp(display),
      wifi_ssid(""),
      wifi_password(""),
      brightness(255),
      currentScreen(Screen::MainMenu),
      settingsActive(false) {  // <-- new flag initialization
}

void MenuInterface::begin() {
  // 1) Load stored WiFi credentials (if they exist)
  prefs.begin("wifi", false);
  wifi_ssid = prefs.getString("ssid", "");
  wifi_password = prefs.getString("pass", "");
  prefs.end();

  // 2) Load stored brightness if saved (optional)
  prefs.begin("disp", false);
  brightness = prefs.getUChar("bri", 255);
  prefs.end();
  disp->setBrightness(brightness);

  // 3) Draw the initial main menu
  disp->clearScreen();
  drawMainMenu();
}

/// ----- Public handle() -----

// NEW: Activate the settings menu (called when lower right is touched)
void MenuInterface::activate() {
  settingsActive = true;
  // Stop time display updates while the settings menu is shown
  extern bool updateTimeScreen;
  updateTimeScreen = false;
  drawMainMenu();
}

void MenuInterface::handle() {
  // If settings menu is not active, check if “SET” button is touched
  if (!settingsActive) {
    if (disp->isTouched()) {
      uint16_t tx, ty;
      disp->getTouchPoint(&tx, &ty);
      // Region corresponding to lower-right "SET" button (e.g. 270,200 to 320,240)
      if (tx >= 270 && ty >= 200) {
        activate();
        clearTouchBuffer();
      }
    }
    return;
  }

  if (!disp->isTouched()) {
    return;
  }

  // Debounce: read one point, then wait for release
  uint16_t tx, ty;
  disp->getTouchPoint(&tx, &ty);
  clearTouchBuffer();  // wait until user lifts finger

  switch (currentScreen) {
    case Screen::MainMenu:
      handleMainMenuTouch(tx, ty);
      break;
    case Screen::WiFiMenu:
      handleWiFiMenuTouch(tx, ty);
      break;
    case Screen::BrightnessMenu:
      handleBrightnessMenuTouch(tx, ty);
      break;
    case Screen::AboutScreen:
      handleAboutScreenTouch(tx, ty);
      break;
    case Screen::EnterSSID:
      // Should never fall through: showKeyboard() is blocking until complete
      break;
    case Screen::EnterPassword:
      // Likewise blocking
      break;
    case Screen::Connecting:
      // While connecting, ignore touches or perhaps allow “Cancel”
      break;
  }
}

/// ----- Drawing Helpers -----

static const uint16_t BTN_BG = BLUE;
static const uint16_t BTN_FG = WHITE;
static const uint16_t BTN_TEXT_SZ = 2;

/**
 * @brief Draws the main menu with a grid of four buttons.
 */
void MenuInterface::drawMainMenu() {
  disp->clearScreen();
  // New grid layout (each button drawn as a rectangle):
  // Button 1: WiFi Settings (rect: x=20,y=40,w=130,h=60)
  disp->getTft()->fillRect(20, 40, 130, 60, BTN_BG);
  disp->drawCenteredText("WiFi Settings", 55, BTN_FG, 2);
  // Button 2: Brightness (rect: x=170,y=40,w=130,h=60)
  disp->getTft()->fillRect(170, 40, 130, 60, BTN_BG);
  disp->drawCenteredText("Brightness", 55, BTN_FG, 2);
  // Button 3: About (rect: x=20,y=120,w=130,h=60)
  disp->getTft()->fillRect(20, 120, 130, 60, BTN_BG);
  disp->drawCenteredText("About", 145, BTN_FG, 2);
  // Button 4: Exit (rect: x=170,y=120,w=130,h=60) 
  disp->getTft()->fillRect(170, 120, 130, 60, BTN_BG);
  disp->drawCenteredText("Exit", 145, BTN_FG, 2);
}

/**
 * @brief Draws the WiFi settings menu (SSID, Password, Connect, Back).
 */
void MenuInterface::drawWiFiMenu() {
  disp->clearScreen();
  // Display current SSID
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(CYAN);
  disp->drawCenteredText("Current SSID:", 40, CYAN, 2);
  disp->drawCenteredText(wifi_ssid.length() > 0 ? wifi_ssid : "(none)", 60, WHITE, 2);

  // “Change SSID” button
  disp->getTft()->fillRect(20, 90, 280, 40, BTN_BG);
  disp->drawCenteredText("Change SSID", 115, BTN_FG, 2);

  // “Change Password” button
  disp->getTft()->fillRect(20, 150, 280, 40, BTN_BG);
  disp->drawCenteredText("Change Password", 175, BTN_FG, 2);

  // “Connect” button
  disp->getTft()->fillRect(20, 210, 280, 40, RED);
  disp->drawCenteredText("Connect", 235, WHITE, 2);

  // “Back” button (top‐left corner)
  disp->getTft()->fillRect(0, 0, 50, 30, DARKGRAY);
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(WHITE);
  disp->getTft()->setCursor(5, 5);
  disp->getTft()->print("<");
}

/**
 * @brief Draws the brightness menu with 3 preset buttons and a slider bar.
 */
void MenuInterface::drawBrightnessMenu() {
  disp->clearScreen();
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(CYAN);
  disp->drawCenteredText("Brightness", 20, CYAN, 2);

  // Preset buttons: 25%, 50%, 100%
  // 25%
  disp->getTft()->fillRect(20, 50, 80, 40, BTN_BG);
  disp->drawCenteredText("25%", 75, BTN_FG, 2);
  // 50%
  disp->getTft()->fillRect(120, 50, 80, 40, BTN_BG);
  disp->drawCenteredText("50%", 155, BTN_FG, 2);
  // 100%
  disp->getTft()->fillRect(220, 50, 80, 40, BTN_BG);
  disp->drawCenteredText("100%", 255, BTN_FG, 2);

  // Slider bar outline
  disp->getTft()->drawRect(20, 110, 280, 30, WHITE);
  // Slider knob (simple: fill a rectangle whose width = brightness/255 * 280)
  uint16_t knobWidth = map(brightness, 0, 255, 0, 280);
  disp->getTft()->fillRect(20, 110, knobWidth, 30, GREEN);

  // “Back” button
  disp->getTft()->fillRect(0, 0, 50, 30, DARKGRAY);
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(WHITE);
  disp->getTft()->setCursor(5, 5);
  disp->getTft()->print("<");
}

/**
 * @brief Draws the “About” screen.
 */
void MenuInterface::drawAboutScreen() {
  disp->clearScreen();
  disp->getTft()->setTextColor(WHITE);
  disp->getTft()->setTextSize(2);
  disp->drawCenteredText("About This Device", 40, CYAN, 2);
  disp->getTft()->setTextSize(1);
  disp->drawCenteredText("ESP32‐S3 Touch Interface", 80, WHITE, 1);
  disp->drawCenteredText("Version 1.0", 100, WHITE, 1);
  disp->drawCenteredText("By Carson (ECEN)", 120, WHITE, 1);

  // “Back” button
  disp->getTft()->fillRect(0, 0, 50, 30, DARKGRAY);
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(WHITE);
  disp->getTft()->setCursor(5, 5);
  disp->getTft()->print("<");
}

/// ----- Touch Handlers -----

/**
 * @brief Handles a touch at (x,y) when on MainMenu.
 */
void MenuInterface::handleMainMenuTouch(uint16_t x, uint16_t y) {
  // Button 1: WiFi Settings (20≤x≤150, 40≤y≤100)
  if (x >= 20 && x <= 150 && y >= 40 && y <= 100) {
    currentScreen = Screen::WiFiMenu;
    drawWiFiMenu();
    return;
  }
  // Button 2: Brightness (170≤x≤300, 40≤y≤100)
  if (x >= 170 && x <= 300 && y >= 40 && y <= 100) {
    currentScreen = Screen::BrightnessMenu;
    drawBrightnessMenu();
    return;
  }
  // Button 3: About (20≤x≤150, 120≤y≤180)
  if (x >= 20 && x <= 150 && y >= 120 && y <= 180) {
    currentScreen = Screen::AboutScreen;
    drawAboutScreen();
    return;
  }
  // Button 4: Exit (170≤x≤300, 120≤y≤180)
  if (x >= 170 && x <= 300 && y >= 120 && y <= 180) {
    settingsActive = false;
    // Restart time display updates after exiting settings
    extern bool updateTimeScreen;
    updateTimeScreen = true;
    disp->clearScreen();
    return;
  }
}

/**
 * @brief Handles a touch on the WiFi settings screen.
 */
void MenuInterface::handleWiFiMenuTouch(uint16_t x, uint16_t y) {
  // “Back” (0≤x≤50, 0≤y≤30)
  if (x <= 50 && y <= 30) {
    currentScreen = Screen::MainMenu;
    drawMainMenu();
    return;
  }
  // “Change SSID” (20≤x≤300, 90≤y≤130)
  if (x >= 20 && x <= 300 && y >= 90 && y <= 130) {
    currentScreen = Screen::EnterSSID;
    // Show keyboard; block until complete
    tempInput = showKeyboard("Enter new SSID:", wifi_ssid);
    if (tempInput.length() > 0) {
      wifi_ssid = tempInput;
      prefs.begin("wifi", false);
      prefs.putString("ssid", wifi_ssid);
      prefs.end();
    }
    currentScreen = Screen::WiFiMenu;
    drawWiFiMenu();
    return;
  }
  // “Change Password” (20≤x≤300, 150≤y≤190)
  if (x >= 20 && x <= 300 && y >= 150 && y <= 190) {
    currentScreen = Screen::EnterPassword;
    tempInput = showKeyboard("Enter new Password:", wifi_password);
    if (tempInput.length() > 0) {
      wifi_password = tempInput;
      prefs.begin("wifi", false);
      prefs.putString("pass", wifi_password);
      prefs.end();
    }
    currentScreen = Screen::WiFiMenu;
    drawWiFiMenu();
    return;
  }
  // “Connect” (20≤x≤300, 210≤y≤250)
  if (x >= 20 && x <= 300 && y >= 210 && y <= 250) {
    currentScreen = Screen::Connecting;
    disp->showMessage("Connecting...", "");
    bool ok = attemptWiFiConnect(wifi_ssid, wifi_password);
    if (ok) {
      disp->showMessage("WiFi Connected!", WiFi.localIP().toString());
    } else {
      disp->showMessage("Connection Failed", "");
    }
    delay(1500);
    currentScreen = Screen::WiFiMenu;
    drawWiFiMenu();
    return;
  }
}

/**
 * @brief Handles a touch on the Brightness screen.
 */
void MenuInterface::handleBrightnessMenuTouch(uint16_t x, uint16_t y) {
  // “Back” (0≤x≤50, 0≤y≤30)
  if (x <= 50 && y <= 30) {
    currentScreen = Screen::MainMenu;
    drawMainMenu();
    return;
  }
  // Presets: 25% (20≤x≤100, 50≤y≤90)
  if (x >= 20 && x <= 100 && y >= 50 && y <= 90) {
    brightness = 64;
  }
  // 50% (120≤x≤200, 50≤y≤90)
  else if (x >= 120 && x <= 200 && y >= 50 && y <= 90) {
    brightness = 128;
  }
  // 100% (220≤x≤300, 50≤y≤90)
  else if (x >= 220 && x <= 300 && y >= 50 && y <= 90) {
    brightness = 255;
  }
  // Slider bar region: 20≤x≤300, 110≤y≤140
  else if (x >= 20 && x <= 300 && y >= 110 && y <= 140) {
    // Map x to brightness
    brightness = map(x, 20, 300, 0, 255);
  } else {
    // Touched outside interactive areas; do nothing
    return;
  }
  // Apply the new brightness immediately and redraw
  disp->setBrightness(brightness);
  // Save to Preferences
  prefs.begin("disp", false);
  prefs.putUChar("bri", brightness);
  prefs.end();
  drawBrightnessMenu();
}

/**
 * @brief Handles a touch on the About screen (only “Back” exists).
 */
void MenuInterface::handleAboutScreenTouch(uint16_t x, uint16_t y) {
  if (x <= 50 && y <= 30) {
    currentScreen = Screen::MainMenu;
    drawMainMenu();
  }
}

/// ----- On‐Screen Keyboard Implementation -----

/**
 * @brief Presents a simple on‐screen keyboard for text entry.
 *
 * This routine blocks until the user taps “OK” or “Cancel.”  
 * It returns the entered string (or the original if Cancel).  
 *
 * @param prompt      A short message at the top (e.g., “Enter SSID:”).
 * @param initial     Initial text to populate (user can delete/append).
 * @return String     The final text (empty if user pressed Cancel and no initial).
 */
String MenuInterface::showKeyboard(const String& prompt, const String& initial) {
  // Layout: We'll do a 3‐row alphabet + digits keyboard, plus “Space”, “Backspace”, “OK”, “Cancel”.
  // For brevity, we define key labels in a static array.
  const char* keys[] = {
      "Q","W","E","R","T","Y","U","I","O","P",
      "A","S","D","F","G","H","J","K","L",
      "Z","X","C","V","B","N","M",
      "0","1","2","3","4","5","6","7","8","9",
      "SPACE","⌫","OK","CANCEL"
  };
  const uint8_t nKeys = sizeof(keys) / sizeof(keys[0]);

  // Calculate positions for keys: 10 columns max, variable rows
  // We'll define rectangles for each key.
  struct KRect { int16_t x, y, w, h; };
  static KRect krs[40];  // enough for our ~36 keys
  memset(krs, 0, sizeof(krs));

  uint8_t idx = 0;
  // Draw prompt and initial text at top
  disp->clearScreen();
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(YELLOW);
  disp->drawCenteredText(prompt, 20, YELLOW, 2);

  // Display the current input on a dedicated line:
  uint16_t inputY = 50;
  disp->getTft()->setTextSize(2);
  disp->getTft()->setTextColor(WHITE);
  // If initial is non‐empty, show it now:
  disp->drawCenteredText(initial, inputY, WHITE, 2);

  // Build all keys, row by row
  // Row 0: keys[0]–keys[9] (10 keys)
  // Row 1: keys[10]–keys[18] (9 keys)
  // Row 2: keys[19]–keys[25] (7 keys)
  // Row 3: keys[26]–keys[35] (10 keys: digits 0–9)
  // Row 4: keys[36]–keys[39] (“SPACE”, “⌫”, “OK”, “CANCEL”)
  //
  // All keys are 28×30 px, with 4 px spacing. Center them horizontally.

  const uint16_t keyW = 28, keyH = 30, keyGap = 4;
  uint16_t screenW = 320;
  // Precompute row widths:
  uint8_t rowCounts[] = {10, 9, 7, 10, 4};
  uint16_t rowY[] = {80, 80 + (keyH + keyGap), 80 + 2 * (keyH + keyGap),
                     80 + 3 * (keyH + keyGap), 80 + 4 * (keyH + keyGap)};

  for (uint8_t row = 0; row < 5; row++) {
    uint8_t count = rowCounts[row];
    uint16_t totalRowW = count * keyW + (count - 1) * keyGap;
    int16_t startX = (screenW - totalRowW) / 2;
    for (uint8_t c = 0; c < count; c++) {
      int16_t kx = startX + c * (keyW + keyGap);
      int16_t ky = rowY[row];
      krs[idx] = {kx, ky, keyW, keyH};
      // Draw key background
      disp->getTft()->fillRect(kx, ky, keyW, keyH, DARKGRAY);
      // Draw key label centered inside
      disp->getTft()->setTextSize(1);
      disp->getTft()->setTextColor(WHITE);
      // Compute text width/height
      int16_t tx1, ty1;
      uint16_t tw, th;
      disp->getTft()->getTextBounds(keys[idx], 0, 0, &tx1, &ty1, &tw, &th);
      int16_t tx = kx + (keyW - tw) / 2;
      int16_t ty = ky + (keyH - th) / 2;
      disp->getTft()->setCursor(tx, ty);
      disp->getTft()->print(keys[idx]);
      idx++;
    }
  }

  // Now, loop until user taps “OK” or “CANCEL”
  String current = initial;
  while (true) {
    // Show the current input text on that line (erase previous: draw black rect then re‐draw)
    disp->getTft()->fillRect(0, inputY - 5, 320, 25, BLACK);
    disp->getTft()->setTextSize(2);
    disp->getTft()->setTextColor(WHITE);
    disp->drawCenteredText(current, inputY, WHITE, 2);

    // Wait for a tap
    while (!disp->isTouched()) {
      delay(10);
    }
    uint16_t tx, ty;
    disp->getTouchPoint(&tx, &ty);
    clearTouchBuffer();

    // Find which key was tapped
    for (uint8_t i = 0; i < nKeys; i++) {
      KRect& r = krs[i];
      if (tx >= r.x && tx <= (r.x + r.w) && ty >= r.y && ty <= (r.y + r.h)) {
        String key = String(keys[i]);
        if (key == "OK") {
          return current;
        }
        if (key == "CANCEL") {
          return initial;
        }
        if (key == "⌫") {
          if (current.length() > 0) {
            current.remove(current.length() - 1);
          }
        } else if (key == "SPACE") {
          current += ' ';
        } else {
          // Single letter or digit
          current += key;
        }
        break;
      }
    }
  }
}

/// ----- WiFi Connection Helper -----

/**
 * @brief Attempts to connect to a WiFi network (blocking, with timeout).
 *
 * Draws status messages on screen.  
 * @param ssid    SSID to connect to.
 * @param pass    Password to use.
 * @return true   if connected successfully (within 10s), false otherwise.
 */
bool MenuInterface::attemptWiFiConnect(const String& ssid, const String& pass) {
  if (ssid.length() == 0) {
    disp->showMessage("No SSID Set", "");
    delay(1000);
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  // Wait up to 10 seconds
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
    disp->showMessage("Connecting...", String((millis() - start) / 1000) + "s");
    delay(500);
  }
  return (WiFi.status() == WL_CONNECTED);
}

/// ----- Utility -----

/**
 * @brief Blocks until the user lifts their finger (no touch detected).
 */
void MenuInterface::clearTouchBuffer() {
  while (disp->isTouched()) {
    delay(10);
  }
}
