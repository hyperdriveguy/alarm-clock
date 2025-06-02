// ntp_clock64.cpp
#include "ntp_clock64.h"

NTPClock64::NTPClock64(const char* ssid,
                       const char* password,
                       const char* posixTZ,
                       const char* ntpServer,
                       uint32_t    syncInterval)
 : ssid_(ssid),
   password_(password),
   tz_(posixTZ),
   ntpServer_(ntpServer),
   syncInterval_(syncInterval),
   offlineMode_(false),
   offlineBaseTime_(0),
   offlineStartMillis_(0)
{}

void NTPClock64::begin() {
  // Attempt WiFi connect with a 10-second timeout
  WiFi.begin(ssid_, password_);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < 10000)) {
    delay(500);
  }
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed; switching to offline mode");
    offlineMode_ = true;
    // Set offline base; use a default starting time
    offlineBaseTime_ = 1704067200; // e.g. Jan 1, 2024 (Unix time)
    offlineStartMillis_ = millis();
    return;
  }

  // Set TZ and do initial SNTP
  configTzTime(tz_, ntpServer_);
  struct tm tm;
  start = millis();
  while (!getLocalTime(&tm) && (millis() - start < 10000)) {
    delay(200);
  }
  if (!getLocalTime(&tm)) {
    Serial.println("SNTP failed; switching to offline mode");
    offlineMode_ = true;
    offlineBaseTime_ = 1704067200; // default offline time (Jan 1, 2024)
    offlineStartMillis_ = millis();
    return;
  }
  offlineMode_ = false;
  lastSync_ = millis() / 1000;
}

void NTPClock64::sync() {
  if (!offlineMode_) {
    configTzTime(tz_, ntpServer_);
    lastSync_ = millis() / 1000;
  }
}

bool NTPClock64::update() {
  if (!offlineMode_) {
    uint32_t now = millis() / 1000;
    if (now - lastSync_ >= syncInterval_) {
      sync();
      return true;
    }
  }
  // In offline mode, nothing to update
  return false;
}

time_t NTPClock64::nowUTC() const {
  if (offlineMode_) {
    return offlineBaseTime_ + ((millis() - offlineStartMillis_) / 1000);
  }
  return time(nullptr);
}

time_t NTPClock64::nowLocal() const {
  if (offlineMode_) {
    return offlineBaseTime_ + ((millis() - offlineStartMillis_) / 1000);
  }
  struct tm tm;
  if (getLocalTime(&tm)) {
    return mktime(&tm);
  }
  return nowUTC();
}

String NTPClock64::nowString() const {
  struct tm tm;
  getLocalTime(&tm);
  char buf[20];
  snprintf(buf, sizeof(buf),
           "%04d-%02d-%02d %02d:%02d:%02d",
           tm.tm_year + 1900,
           tm.tm_mon  + 1,
           tm.tm_mday,
           tm.tm_hour,
           tm.tm_min,
           tm.tm_sec);
  return String(buf);
}

ClockDateTime NTPClock64::nowSplit() const {
  time_t currentTime;
  if (offlineMode_) {
    currentTime = offlineBaseTime_ + ((millis() - offlineStartMillis_) / 1000);
  } else {
    struct tm tm;
    if (!getLocalTime(&tm)) {
      currentTime = nowUTC();
    } else {
      currentTime = mktime(&tm);
    }
  }
  struct tm *lt = localtime(&currentTime);
  return {
    lt->tm_year + 1900,
    lt->tm_mon + 1,
    lt->tm_mday,
    lt->tm_hour,
    lt->tm_min,
    lt->tm_sec
  };
}

