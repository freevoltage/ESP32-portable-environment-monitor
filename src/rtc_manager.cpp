#include "rtc_manager.h"
#include <sys/time.h>

// RTC memory variable (survives deep sleep)
RTC_DATA_ATTR bool rtcInitialized = false;

RTCManager::RTCManager() {
    // Constructor
}

void RTCManager::begin(int year, int month, int day, int hour, int minute, int second) {
    setSystemTime(year, month, day, hour, minute, second);
    rtcInitialized = true;
    Serial.println("RTC initialized!");
}

bool RTCManager::isInitialized() {
    return rtcInitialized;
}

void RTCManager::setInitialized(bool state) {
    rtcInitialized = state;
}

void RTCManager::setSystemTime(int year, int month, int day, int hour, int minute, int second) {
    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = -1;
    
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
}

String RTCManager::getFormattedDateTime() {
    time_t now;
    struct tm timeinfo;
    char buffer[64];
    
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    return String(buffer);
}

String RTCManager::getFormattedTime() {
    time_t now;
    struct tm timeinfo;
    char buffer[32];
    
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    
    return String(buffer);
}

String RTCManager::getFormattedDate() {
    time_t now;
    struct tm timeinfo;
    char buffer[32];
    
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
    
    return String(buffer);
}

time_t RTCManager::getTimestamp() {
    time_t now;
    time(&now);
    return now;
}

void RTCManager::getTimeComponents(int &year, int &month, int &day, 
                                   int &hour, int &minute, int &second) {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    year = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;
    day = timeinfo.tm_mday;
    hour = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    second = timeinfo.tm_sec;
}

// RTC memory variable for last sync time
RTC_DATA_ATTR time_t lastNTPSync = 0;

void RTCManager::setLastSyncTime(time_t syncTime) {
    lastNTPSync = syncTime;
}

time_t RTCManager::getLastSyncTime() {
    return lastNTPSync;
}

bool RTCManager::needsTimeSync(int intervalHours) {
    if (lastNTPSync == 0) {
        return true; // Never synced
    }
    
    time_t now = getTimestamp();
    time_t elapsed = now - lastNTPSync;
    time_t intervalSeconds = intervalHours * 3600;
    
    return (elapsed >= intervalSeconds);
}
