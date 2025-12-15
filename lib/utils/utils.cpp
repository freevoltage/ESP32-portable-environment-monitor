#include "utils.h"
#include <logger.h>


/// @brief Parses a datetime string into a Unix timestamp
/// @param dateTimeStr DateTime string in format "YYYY-MM-DD HH:MM:SS" (e.g., "2024-01-15 10:30:45")
/// @return Unix timestamp (seconds since epoch) or 0 if parsing fails
/// @note Returns 0 on parsing errors. Check console output for detailed error messages.
/// @see DateTimeUtils::parseComponents() for the underlying parsing logic
time_t DateTimeUtils::parseDateTime(const String& dateTimeStr) {
    // Parse "2024-01-15 10:30:45"
    int year, month, day, hour, minute, second;
    
    if (!parseComponents(dateTimeStr, year, month, day, hour, minute, second)) {
        LOG_ERROR("Failed to parse datetime: %s\n", dateTimeStr.c_str());
        return 0;
    }
    
    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;  // tm_year is years since 1900
    timeinfo.tm_mon = month - 1;     // tm_mon is 0-11
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = -1;          // Let system determine DST
    
    return mktime(&timeinfo);
}

String DateTimeUtils::convertTimestampToString(time_t timestamp){
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo)); // fills timeinfo with zeros.

    // convert timestamp into tm structure
    // gmtime_r converts the given time since epoch (time_t) to calender time expressed in the struct tm format.
    // the conversion is saved into the struct &timeinfo. Returns nullptr on fail.
    if(gmtime_r(&timestamp, &timeinfo) == nullptr){
        LOG_ERROR("gmtime_r failed for timestamp %ld\n", (long)timestamp);
        return String();  // Return empty string on failure
    }

    // Format as "YYYY-MM-DD HH:MM:SS"
    char buffer[20];
    size_t result = strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);


    if (result == 0) {
        LOG_ERROR("strftime failed");
        return String();  // Return empty string on failure
    }

    return String(buffer);
}

bool DateTimeUtils::parseComponents(const String& str, int& year, int& month, 
                                   int& day, int& hour, int& min, int& sec) {
    // Parse "2024-01-15 10:30:45"
    return sscanf(str.c_str(), "%d-%d-%d %d:%d:%d", 
                  &year, &month, &day, &hour, &min, &sec) == 6;
}

String DateTimeUtils::formatDateTime(time_t timestamp) {
    struct tm* timeinfo = localtime(&timestamp);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}