// rtc_manager.h
#pragma once
#ifdef MOCK
    #include <string>
    #include <ctime>
    typedef std::string String;
#else
    #include <Arduino.h>
    #include <ESP32Time.h>
#endif

class RTCManager {
public:
    RTCManager();

    // Initialize RTC
    bool begin();
    bool isReady() const;

    // Core Time operations
    bool setTime(int year, int month, int day, int hour, int min, int sec); // TODO Make this a void
    bool setTime(unsigned long epoch = 1609459200, int ms = 0); // TODO Implement Test for this function // TODO Make this a void
    time_t getEpochTime();
    String getTime(); // TODO Implement Test for this function
    String getFormattedTime(const String& format = "%Y-%m-%d %H:%M:%S");
    tm getTimeStruct();

    // NTP sync management  
    void setLastSyncTime(time_t syncTime);
    time_t getLastSyncTime() const;
    bool needsTimeSync(int defaultIntervalHours = 24);

    bool isTimeSet();  // Check if time is reasonable (not 1970)
    // Prints all available RTC Formats from the ESP32Lib
    void printRTCFormats();

    static constexpr time_t MIN_VALID_TIME = 1609459200; // 2021-01-01 00:00:00

private:
#ifndef MOCK
    ESP32Time _rtc;
#endif
    time_t _lastSyncTime;
    bool _initialized;
};
