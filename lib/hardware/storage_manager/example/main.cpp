#include <Arduino.h>
#include <storage_manager.h>
#include <data_structures.h>
#include <config.h>

#define LED_PIN LED_BUILTIN

#define WRITE_INTERVAL_MS 2000   // Write every 2 seconds
#define STATUS_INTERVAL   25     // Print summary every N writes

StorageManager storage;
String _filename = "/datalog.csv";

// Write tracking
uint32_t writeCount = 0;
uint32_t failCount = 0;
unsigned long lastWriteTime = 0;
float lastTemp = 20.0;

void debugPrintSetup();

void setup(){
    Serial.begin(115200);
    while(!Serial);
    delay(1000);

    Serial.println("=================================");
    Serial.println("ESP32 BME280 SD Stability Test");
    Serial.println("=================================");

    debugPrintSetup();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // Deselect Display CS Pin on shared SPI bus
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    // Initialize SD Card
    Serial.println("Initializing SD Card...");

    if (!storage.begin()) {
        Serial.println("ERROR: SD card init failed!");
        while (true) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(250);
        }
    }
    Serial.println("SD Card initialized");
    Serial.printf("Writing every %d ms, status every %d writes\n\n",
                  WRITE_INTERVAL_MS, STATUS_INTERVAL);

    // Clear file so we start fresh
    storage.clearFile();

    lastWriteTime = millis();
    digitalWrite(LED_PIN, LOW);
}


void loop(){
    unsigned long now = millis();

    if (now - lastWriteTime < WRITE_INTERVAL_MS) {
        return;
    }
    lastWriteTime = now;

    // Build a reading with pseudo-random-looking values
    lastTemp = 20.0 + (writeCount % 30) * 0.1;  // slowly drifts up, wraps at 30
    float humidity = 50.0 + (writeCount % 50) * 0.2;
    float pressure = 1000.0 + (writeCount % 100);

    SensorReading reading(lastTemp, humidity, pressure, (time_t)(millis() / 1000));
    reading.isValid = true;

    if (storage.storeReading(reading)) {
        writeCount++;
        // Brief LED flash on success
        digitalWrite(LED_PIN, HIGH);
        delay(20);
        digitalWrite(LED_PIN, LOW);
    } else {
        failCount++;
        Serial.printf("FAIL #%lu (total writes: %lu)\n", failCount, writeCount);
    }

    // Periodic status
    if (writeCount > 0 && writeCount % STATUS_INTERVAL == 0) {
        size_t fileSize = storage.getFileSize(_filename);
        Serial.printf("[STATUS] writes=%lu fails=%lu fileSize=%u bytes heap=%u\n",
                      writeCount, failCount, (uint32_t)fileSize, ESP.getFreeHeap());
    }
}


void debugPrintSetup() {
    Serial.println("Debug Info:");
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.println();
}
