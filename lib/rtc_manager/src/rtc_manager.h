// rtc_manager.h
#pragma once
#include <Arduino.h>
#include <ESP32Time.h>

class RTCManager {
public:
    RTCManager();

    // Initialize RTC
    bool begin();
    bool isReady() const;

    // Core Time operations
    bool setTime(int year, int month, int day, int hour, int min, int sec);
    time_t getEpochTime();
    String getFormattedTime(const String& format = "%Y-%m-%d %H:%M:%S");
    tm getTimeStruct();

    // NTP sync management  
    void setLastSyncTime(time_t syncTime);
    time_t getLastSyncTime() const;
    bool needsTimeSync(int defaultIntervalHours = 24);

    bool isTimeSet();  // Check if time is reasonable (not 1970)
    // Prints all available RTC Formats from the ESP32Lib
    void printRTCFormats();

private:
    ESP32Time _rtc;
    time_t _lastSyncTime;
    bool _initialized;

    static constexpr time_t MIN_VALID_TIME = 1640995200; // Jan 1, 2022
};
