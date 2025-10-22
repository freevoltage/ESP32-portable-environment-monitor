#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <time.h>

class RTCManager {
public:
    RTCManager();
    
    // Initialize RTC with specific date/time
    void begin(int year, int month, int day, int hour, int minute, int second);
    
    // Check if RTC has been initialized
    bool isInitialized();
    
    // Mark RTC as initialized (stored in RTC memory)
    void setInitialized(bool state);

    // Track last NTP sync
    void setLastSyncTime(time_t syncTime);
    time_t getLastSyncTime();
    bool needsTimeSync(int intervalHours);
    
    // Get formatted strings
    String getFormattedDateTime();
    String getFormattedTime();
    String getFormattedDate();
    
    // Get Unix timestamp
    time_t getTimestamp();
    
    // Get individual time components
    void getTimeComponents(int &year, int &month, int &day, 
                          int &hour, int &minute, int &second);

private:
    void setSystemTime(int year, int month, int day, 
                      int hour, int minute, int second);
};

#endif
