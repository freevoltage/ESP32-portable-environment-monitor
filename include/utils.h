#pragma once
#include "data_structures.h" // TODO I believe that data structures is not really needed but it also includes arduino.h

// This is a utility class only for reading and formatting the date time string.
// Its very likely that it belongs to its own module
class DateTimeUtils {
public:
    static time_t parseDateTime(const String& dateTimeStr);
    static String formatDateTime(time_t timestamp);
    static bool isValidDateTime(const String& dateTimeStr);
    
private:
    static bool parseComponents(const String& str, int& year, int& month, 
                               int& day, int& hour, int& min, int& sec);
};