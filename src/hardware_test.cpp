#include <Arduino.h>
#include <config.h>
#include <sensor_manager.h>
#include <storage_manager.h>
#include <rtc_manager.h>
#include <display_manager.h>
#include <wifi_manager.h>
#include <data_structures.h>

RTCManager rtc;
SensorManager sensor;
DisplayManager display;
StorageManager storage;
WiFiManager wifiMgr;

#ifndef UNIT_TEST
void setup() {
    Serial.begin(115200);
    while (!Serial);
    delay(2000);

    Serial.println("\n=== Hardware Test ===\n");

    // 1. RTC
    Serial.println("[1/5] RTC...");
    rtc.begin();
    Serial.printf("  Time: %s\n", rtc.getFormattedTime().c_str());

    // 2. WiFi + NTP
    Serial.println("[2/5] WiFi + NTP...");
    if (wifiMgr.connect(WIFI_DEFAULT_SSID, WIFI_DEFAULT_PASSWORD, 10)) {
        Serial.printf("  Connected: %s\n", wifiMgr.getIPAddress().c_str());
        wifiMgr.syncTimeNTP(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);
        time_t now;
        time(&now);
        rtc.setTime(now);
        Serial.printf("  Synced: %s\n", rtc.getFormattedTime().c_str());
        wifiMgr.disconnect();
    } else {
        Serial.println("  FAILED");
    }

    // 3. SD Card (SPI bus conflict avoidance: TFT_CS HIGH before SD init)
    Serial.println("[3/5] SD Card...");
    digitalWrite(TFT_CS, HIGH);
    if (storage.begin()) {
        SensorReading r = {};
        r.temperature = 42.0;
        r.humidity = 50.0;
        r.pressure = 1013.25;
        r.timestamp = rtc.getEpochTime();
        storage.storeReading(r);
        Serial.println("  Write OK");
    } else {
        Serial.println("  FAILED");
    }

    // 4. Display
    Serial.println("[4/5] Display...");
    display.begin();
    display.showMessage("HW TEST OK");
    Serial.println("  OK");

    // 5. BME280 Sensor
    Serial.println("[5/5] BME280...");
    if (sensor.begin()) {
        SensorReading reading = sensor.getReading();
        Serial.printf("  T=%.1fC H=%.1f%% P=%.1fhPa\n",
            reading.temperature, reading.humidity,
            reading.pressure);
    } else {
        Serial.println("  FAILED");
    }

    Serial.println("\n=== Hardware Test Complete ===\n");
}

void loop() {
    delay(10000);
}
#endif
