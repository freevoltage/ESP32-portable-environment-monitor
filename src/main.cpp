#include <Arduino.h>
#include <config.h>
#include <rtc_manager.h>
#include <sensor_manager.h>
#include <display_manager.h>
#include <storage_manager.h>
#include <wifi_manager.h>
#include <connectivity_service.h>
#include <display_service.h>
#include <data_structures.h>

// RTC memory for boot counter
RTC_DATA_ATTR int bootCount = 0;

// Create manager objects
RTCManager rtc;
SensorManager sensor;
DisplayManager display;
StorageManager storage;
WiFiManager wifiMgr;
ConnectivityService connectivity(&wifiMgr, &rtc);
DisplayService displayService(&display, &rtc, &connectivity);

// Timing for sensor refresh
unsigned long lastSensorRead = 0;

void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup: Timer");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup: Button press");
            break;
        default:
            Serial.printf("Wakeup: Power on/Reset (%d)\n", wakeup_reason);
            break;
    }
}

void enterDeepSleep() {
    Serial.println("Entering deep sleep...");
    displayService.turnOff();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.printf("Deep sleep for %d seconds\n", TIME_TO_SLEEP);
    Serial.flush();
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    while(!Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    delay(2000);

    ++bootCount;
    Serial.println("\n================================");
    Serial.printf("Boot #%d\n", bootCount);
    Serial.println("================================");

    printWakeupReason();

    // Sync time via WiFi/NTP
    connectivity.ensureTimeSync();

    Serial.print("Current time: ");
    Serial.println(rtc.getFormattedTime());

    // Initialize SD card first (before display to avoid SPI bus conflict)
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    // Initialize display
    display.begin();

    // Initialize sensor
    if (!sensor.begin()) {
        displayService.showErrorScreen("BME280 sensor not found!");
    }

    // Show startup screen
    displayService.showStartupScreen();
    delay(2000);

    // Take first sensor reading
    SensorReading reading = sensor.getReading();
    if (reading.isValid) {
        displayService.showCurrentReading(reading, rtc.getFormattedTime());
        storage.storeReading(reading);
    }

    lastSensorRead = millis();

    // Configure BOOT button as input with pull-up
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    Serial.println("Ready. Press BOOT button to enter deep sleep.");
}

void loop() {
    // Check if BOOT button pressed (active LOW)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        delay(50); // Debounce
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            Serial.println("BOOT button pressed!");
            enterDeepSleep();
        }
    }

    // Check if it's time for a new sensor reading
    unsigned long now = millis();
    if (now - lastSensorRead >= SENSOR_REFRESH_INTERVAL) {
        SensorReading reading = sensor.getReading();
        if (reading.isValid && displayService.needsUpdate(reading)) {
            displayService.showCurrentReading(reading, rtc.getFormattedTime());
            storage.storeReading(reading);
        }
        lastSensorRead = now;
    }

    delay(100); // Small delay to reduce CPU usage
}
