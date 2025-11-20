#include "utils.h"

time_t DateTimeUtils::parseDateTime(const String& dateTimeStr) {
    // Parse "2024-01-15 10:30:45"
    int year, month, day, hour, minute, second;
    
    if (!parseComponents(dateTimeStr, year, month, day, hour, minute, second)) {
        Serial.printf("Failed to parse datetime: %s\n", dateTimeStr.c_str());
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
