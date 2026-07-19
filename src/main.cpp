#include <Arduino.h>
#include <config.h>
#include <rtc_manager.h>
#include <sensor_manager.h>
#include <display_manager.h>
#include <storage_manager.h>
#include <wifi_manager.h>
#include <connectivity_service.h>
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

void printWakeupReason() {
    // TODO This should be very likely also moved into some module. I think of an sleep_module
    // where additional deep sleep related functionality might be added in the future.
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER: 
            Serial.println("Wakeup: Timer"); 
            break;
        case ESP_SLEEP_WAKEUP_EXT0: 
            Serial.println("Wakeup: External signal"); 
            break;
        default: 
            Serial.printf("Wakeup: Power on/Reset (%d)\n", wakeup_reason); 
            break;
    }
}

void setup() {
    Serial.begin(115200);
    while(!Serial);

    // Indicate Power ON State by Turning LED ON
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    delay(2000); // Allow some time for the serial monitor to connect.
    
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
    digitalWrite(TFT_CS, HIGH);  // Ensure display CS is deselected
    storage.begin();
    
    // Initialize display
    display.begin();
    
    // Initialize sensor
    if (!sensor.begin()) {
        display.showError("BME280 sensor not found!");
    }
    
    // DEBUG List Files of Storage
    storage.listFiles();

    // Read Datalog.csv File
    String buffer;
    if(storage.readFile("/datalog.csv", buffer, DEFAULT_MAX_SIZE)){
        Serial.println("File Content");
        Serial.println(buffer);
    };
    
    // Take sensor reading
    SensorReading reading = sensor.getReading();
    
    // Print to Serial
    //sensor.print(reading);
    // TODO Add a print function to the Display Service Class
    // TODO Actually none of the service classes is even implemented
    Serial.print("DateTime: ");
    Serial.println(rtc.getFormattedTime());
    
    // Display on screen
    // TODO I might want to change that this function doesn't need RTC time. We'll see..
    //display.showReading(reading, rtc.getFormattedDateTime());
    display.showReading(reading);
    
    // Save to SD card
    storage.storeReading(reading);
    
    // Wait before sleep (upload window)
    Serial.println("\nWaiting 10 seconds before sleep...");
    Serial.println("Press BOOT button now to stay awake for upload");
    delay(10000);
    
    // Configure and enter deep sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.printf("Going to deep sleep for %d seconds...\n", TIME_TO_SLEEP);
    Serial.flush();
    
    display.disconnect();
    esp_deep_sleep_start();
}

void loop() {
    // Never reached when using deep sleep
}
