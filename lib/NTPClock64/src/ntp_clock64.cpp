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
   syncInterval_(syncInterval)
{}

void NTPClock64::begin() {
  // 1) WiFi connect
  WiFi.begin(ssid_, password_);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // 2) Set TZ and do initial SNTP
  configTzTime(tz_, ntpServer_);
  
  // 3) Wait for valid time
  struct tm tm;
  while (!getLocalTime(&tm)) {
    delay(200);
  }
  lastSync_ = millis() / 1000;
}

void NTPClock64::sync() {
  configTime(0, 0, ntpServer_);
  lastSync_ = millis() / 1000;
}

bool NTPClock64::update() {
  uint32_t now = millis() / 1000;
  if (now - lastSync_ >= syncInterval_) {
    sync();
    return true;
  }
  return false;
}

time_t NTPClock64::nowUTC() const {
  return time(nullptr);
}

time_t NTPClock64::nowLocal() const {
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
  struct tm tm;
  if (!getLocalTime(&tm)) {
    return {0};  // fallback on failure
  }

  return {
    tm.tm_year + 1900,
    tm.tm_mon + 1,
    tm.tm_mday,
    tm.tm_hour,
    tm.tm_min,
    tm.tm_sec
  };
}

