#include <Arduino.h>
#include <storage_manager.h>
#include <data_structures.h>
#include <config.h>

#define LED_PIN LED_BUILTIN

StorageManager storage;
String _filename = "/datalog.csv";

void setup(){
    Serial.begin(115200);
    while(!Serial);
    delay(1000);

    Serial.println("=================================");
    Serial.println("ESP32 BME280 SD Card Cleaner");
    Serial.println("=================================");

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
    Serial.println("SD Card initialized\n");

    // Show files before
    Serial.println("--- Files BEFORE deletion ---");
    storage.listFiles();
    size_t sizeBefore = storage.getFileSize(_filename);
    Serial.printf("%s size: %u bytes\n\n", _filename.c_str(), (uint32_t)sizeBefore);

    // Delete
    if (storage.fileExists(_filename)) {
        Serial.printf("Deleting %s...\n", _filename.c_str());
        if (storage.deleteFile(_filename)) {
            Serial.println("File deleted successfully.");
        } else {
            Serial.println("ERROR: Delete failed!");
        }
    } else {
        Serial.printf("File %s does not exist, nothing to delete.\n", _filename.c_str());
    }

    // Show files after
    Serial.println("\n--- Files AFTER deletion ---");
    storage.listFiles();
    bool existsAfter = storage.fileExists(_filename);
    Serial.printf("%s exists: %s\n\n", _filename.c_str(), existsAfter ? "YES (ERROR)" : "NO (OK)");

    Serial.println("Done.");
    digitalWrite(LED_PIN, LOW);
}


void loop(){
}
