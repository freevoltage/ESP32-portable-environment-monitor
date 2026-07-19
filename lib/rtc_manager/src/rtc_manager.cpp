#ifndef MOCK
#include "rtc_manager.h"
#include <logger.h>

RTCManager::RTCManager() 
    : _lastSyncTime(0), _initialized(false) {
}

bool RTCManager::begin() {
    LOG_INFO("RTCManager begin()");
    // ESP32Time doesn't have a begin() method, it's ready to use
    // We'll just set a flag and maybe set a default time if needed
    _initialized = true;
    
    // If no valid time is set, set a default time
    if (!isTimeSet()) {
        _rtc.setTime(MIN_VALID_TIME);
    }
    LOG_INFO("RTC initialized successfully");
    return true;
}

bool RTCManager::isReady() const {
    return _initialized;
}

bool RTCManager::setTime(int year, int month, int day, int hour, int min, int sec) {
    if (!_initialized) {
        return false;
    }
    
    _rtc.setTime(sec, min, hour, day, month, year);
    return true;
}


/// @brief 
/// @param epoch 
/// @param ms 
/// @return 
bool RTCManager::setTime(unsigned long epoch, int ms)
{
    if (!_initialized) {
        return false;
    }
    
    _rtc.setTime(epoch, ms);
    LOG_DEBUG("RTC Time set to: %lu, offset: %i (ms), UTC: %s", epoch, ms, this->getFormattedTime().c_str());
    return true;
}

/// @brief Calls the ESP32Time getEpoch function
/// @return the current epoch seconds as time_t
time_t RTCManager::getEpochTime(){
    if (!_initialized) {
        return 0;
    }
    // It's completely save to cast to time_t.
    //For some reason in the ESPTime Library there was a cast from time_t to unsigned long before ...
    return static_cast<time_t>(_rtc.getEpoch());
}

/// @brief 
/// @return the time as an Arduino String object
String RTCManager::getTime()
{
    if(!_initialized){
        return String();
    }
    return _rtc.getTime();
}

String RTCManager::getFormattedTime(const String& format){
    if (!_initialized) {
        return "Not initialized";
    }
    return _rtc.getTime(format);
}

tm RTCManager::getTimeStruct(){
    tm emptyStruct = {};
    if (!_initialized) {
        return emptyStruct; // Return empty struct
    }
    
    return _rtc.getTimeStruct();
    
}

void RTCManager::setLastSyncTime(time_t syncTime) {
    _lastSyncTime = syncTime;
}

time_t RTCManager::getLastSyncTime() const {
    return _lastSyncTime;
}

bool RTCManager::needsTimeSync(int defaultIntervalHours){
    if (!_initialized || _lastSyncTime == 0) {
        return true; // Never synced before
    }
    
    time_t now = getEpochTime();
    time_t intervalSeconds = defaultIntervalHours * 3600;
    
    return (now - _lastSyncTime) >= intervalSeconds;
}

bool RTCManager::isTimeSet(){
    if (!_initialized) {
        return false;
    }
    
    time_t currentTime = getEpochTime();
    return currentTime >= MIN_VALID_TIME;
}

// Prints all available RTC Formats from the ESP32Lib
// For Debugging
void RTCManager::printRTCFormats()
{
    if (!_initialized) {
        LOG_ERROR("RTC not initialized");
        return;
    }

    LOG_INFO("=== RTC Format Examples ===");
    
    // Epoch
    unsigned long epoch = _rtc.getEpoch();
    LOG_INFO("Epoch: %lu", epoch);
    
    // Time struct
    struct tm timeinfo = _rtc.getTimeStruct();
    LOG_INFO("Time struct - Year: %d, Month: %d, Day: %d, Hour: %d, Min: %d, Sec: %d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    // Default time
    LOG_INFO("getTime(): %s", _rtc.getTime().c_str());
    
    // Custom format
    LOG_INFO("getTime(\"%%Y-%%m-%%d %%H:%%M:%%S\"): %s", 
             _rtc.getTime("%Y-%m-%d %H:%M:%S").c_str());
    
    // Date/Time combinations
    LOG_INFO("getDateTime(false): %s", _rtc.getDateTime(false).c_str());
    LOG_INFO("getDateTime(true): %s", _rtc.getDateTime(true).c_str());
    LOG_INFO("getTimeDate(false): %s", _rtc.getTimeDate(false).c_str());
    LOG_INFO("getTimeDate(true): %s", _rtc.getTimeDate(true).c_str());
    
    // Date only
    LOG_INFO("getDate(false): %s", _rtc.getDate(false).c_str());
    LOG_INFO("getDate(true): %s", _rtc.getDate(true).c_str());
    
    // AM/PM
    LOG_INFO("getAmPm(false): %s", _rtc.getAmPm(false).c_str());
    LOG_INFO("getAmPm(true): %s", _rtc.getAmPm(true).c_str());
    
    LOG_INFO("=========================");
}

#endif // !MOCK
