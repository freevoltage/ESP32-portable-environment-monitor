#include <Arduino.h>
#include "config.h"
#include "rtc_manager.h"
#include "sensor.h"
#include "display.h"
#include "storage.h"
#include "wifi_manager.h"

// RTC memory for boot counter
RTC_DATA_ATTR int bootCount = 0;

// Create manager objects
RTCManager rtc;
SensorManager sensor;
DisplayManager display;
StorageManager storage;
WiFiManager wifiMgr;

void printWakeupReason() {
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

void syncTime() {
    // Check if time sync is needed
    if (!rtc.needsTimeSync(TIME_SYNC_INTERVAL_HOURS)) {
        Serial.println("Time sync not needed yet");
        time_t lastSync = rtc.getLastSyncTime();
        Serial.printf("Last sync: %ld seconds ago\n",
                      rtc.getTimestamp() - lastSync);
        return;
    }
    
    Serial.println("Time sync needed - connecting to WiFi...");
    
    // Connect to WiFi
    if (wifiMgr.connect(WIFI_SSID, WIFI_PASSWORD, 20)) {
        // Sync time from NTP
        if (wifiMgr.syncTimeNTP(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC)) {
            rtc.setLastSyncTime(rtc.getTimestamp());
            rtc.setInitialized(true);
            Serial.println("Time synchronized and saved");
        } else {
            Serial.println("Time sync failed");
        }
        
        // Disconnect WiFi to save power
        wifiMgr.disconnect();
    } else {
        Serial.println("WiFi connection failed - using existing time");
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
    //syncTime();

    Serial.print("Current time: ");
    Serial.println(rtc.getFormattedDateTime());
    
    // Initialize display
    display.begin(TFT_CS, TFT_DC, TFT_RST, TFT_LIT);
    
    // Initialize sensor
    if (!sensor.begin()) {
        display.showError("BME280 sensor not found!");
    }
    
    // Initialize SD card
    storage.begin();
    
    // DEBUG List Files of Storage
    storage.listFiles();

    // Read Datalog.csv File
    String buffer;
    if(storage.readFile("/datalog.csv", buffer)){
        Serial.println("File Content");
        Serial.println(buffer);
    };
    

    // Take sensor reading
    SensorReading reading = sensor.takeReading();
    
    // Print to Serial
    sensor.printReading(reading);
    Serial.print("DateTime: ");
    Serial.println(rtc.getFormattedDateTime());
    
    // Display on screen
    display.showReading(reading, rtc.getFormattedDateTime());
    display.showBootCount(bootCount);
    
    // Save to SD card
    storage.writeReading(reading, rtc.getFormattedDateTime());
    
    // Wait before sleep (upload window)
    Serial.println("\nWaiting 10 seconds before sleep...");
    Serial.println("Press BOOT button now to stay awake for upload");
    delay(10000);
    
    // Configure and enter deep sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.printf("Going to deep sleep for %d seconds...\n", TIME_TO_SLEEP);
    Serial.flush();
    
    //digitalWrite(LED_BUILTIN, LOW); // Would be LOW anyway, because the peripheral shuts down
    display.disconnect();
    //esp_deep_sleep_start();
}

void loop() {
    // Never reached when using deep sleep
}
