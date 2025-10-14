#include <Arduino.h>
#include <ESP32Time.h>

ESP32Time rtc(0);  // offset in seconds from GMT

void setup() {
    Serial.begin(115200);
    while(!Serial);
    Serial.println("ESP32-C6 RTC Example");

    // Set the RTC time (year, month, day, hour, minute, second)
    // January 1st, 2024, 12:00:00
    rtc.setTime(0, 0, 12, 1, 1, 2024);  // sec, min, hour, day, month, year
    
    Serial.println("RTC initialized!");
}

void loop() {
    Serial.println(rtc.getTime());          // (String) 15:24:38
    Serial.println(rtc.getDate());          // (String) Sun, Jan 17 2021
    Serial.println(rtc.getDateTime());      // (String) Sun, Jan 17 2021 15:24:38
    Serial.println(rtc.getTimeDate());      // (String) 15:24:38 Sun, Jan 17 2021
    
    Serial.print("Epoch: ");
    Serial.println(rtc.getEpoch());         // (unsigned long) 1609459200
    
    Serial.print("Millis: ");
    Serial.println(rtc.getMillis());        // (unsigned long) 1609459200000
    
    delay(1000);
}
