// ntp_clock64.h
#ifndef NTP_CLOCK64_H
#define NTP_CLOCK64_H

#include <WiFi.h>
#include <time.h>

struct ClockDateTime {
  int year;
  int month;   // 1–12
  int day;     // 1–31
  int hour;    // 0–23
  int minute;  // 0–59
  int second;  // 0–59
};

/**
 * @class NTPClock64
 * @brief 64-bit NTP clock with auto-DST and periodic re-sync.
 */
class NTPClock64 {
 public:
  /**
   * @brief Construct a new NTPClock64.
   * @param ssid          WiFi SSID
   * @param password      WiFi password
   * @param posixTZ       POSIX TZ string (e.g. "MST7MDT,M3.2.0/2,M11.1.0/2")
   * @param ntpServer     NTP server hostname (default = pool.ntp.org)
   * @param syncInterval  Re-sync interval in seconds (default = 3600)
   */
  NTPClock64(const char* ssid,
             const char* password,
             const char* posixTZ,
             const char* ntpServer    = "pool.ntp.org",
             uint32_t    syncInterval = 3600u);

  /** @brief Initialize WiFi, set TZ, do first sync. */
  void begin();

  /** @brief Force an immediate NTP re-sync. */
  void sync();

  /**
   * @brief Call this regularly (e.g. in loop()) to auto-sync when needed.
   * @return true  if a sync was performed this call
   * @return false if no sync was due yet
   */
  bool  update();

  /** @brief Seconds since Unix epoch (UTC). */
  time_t nowUTC() const;

  /** @brief Seconds since Unix epoch (local, with DST). */
  time_t nowLocal() const;

  /** @brief Formatted "YYYY-MM-DD HH:MM:SS" in local time. */
  String nowString() const;

  ClockDateTime nowSplit() const;

 private:
  const char* ssid_;
  const char* password_;
  const char* tz_;
  const char* ntpServer_;
  uint32_t    syncInterval_;
  uint32_t    lastSync_ = 0;
};

#endif  // NTP_CLOCK64_H
