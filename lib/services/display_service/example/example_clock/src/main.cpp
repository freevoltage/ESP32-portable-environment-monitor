// Clock Example — Service Layer
// Uses ConnectivityService (WiFi + NTP) and DisplayService to show a live clock.
// Also prints time to serial as fallback.

#include <Arduino.h>
#include <config.h>
#include <display_manager.h>
#include <rtc_manager.h>
#include <wifi_manager.h>
#include <connectivity_service.h>
#include <display_service.h>

DisplayManager display;
RTCManager rtc;
WiFiManager wifiMgr;
ConnectivityService connectivity(&wifiMgr, &rtc);
DisplayService displayService(&display, &rtc, &connectivity);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("\n=== Clock Example ===\n");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Init display
    digitalWrite(TFT_CS, HIGH);
    display.begin();
    display.setBrightness(255);

    // Init RTC
    rtc.begin();

    // Connect WiFi and sync time
    display.showMessage("WiFi...");
    Serial.println("Connecting to WiFi...");
    connectivity.ensureTimeSync();

    if (rtc.isTimeSet()) {
        Serial.printf("Time synced: %s\n", rtc.getFormattedTime().c_str());
    } else {
        Serial.println("Time not set — clock will show default");
    }

    display.clear();
}

void loop() {
    if (rtc.isTimeSet()) {
        String timeStr = rtc.getFormattedTime("%H:%M:%S");
        String dateStr = rtc.getFormattedTime("%Y-%m-%d");
        displayService.showClock(timeStr, dateStr);
        Serial.printf("[%s] %s\n", dateStr.c_str(), timeStr.c_str());
    } else {
        display.showMessage("Waiting for time...");
        Serial.println("Waiting for time sync...");
    }

    delay(1000);
}
