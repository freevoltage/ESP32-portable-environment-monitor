#include <Arduino.h>
#include <storage_manager.h>
#include <data_structures.h>
#include <config.h>

#define LED_PIN LED_BUILTIN
#define READ_BUFFER_SIZE 32768

StorageManager storage;
String _filename = "/datalog.csv";

void setup(){
    Serial.begin(115200);
    while(!Serial);
    delay(1000);

    Serial.println("=================================");
    Serial.println("ESP32 BME280 SD Card Reader");
    Serial.println("=================================");
    Serial.printf("Free Heap: %d bytes\n\n", ESP.getFreeHeap());

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

    // List all files
    Serial.println("--- Files on SD Card ---");
    storage.listFiles();
    Serial.println();

    // Read file content
    Serial.printf("--- Content of %s ---\n", _filename.c_str());
    size_t fileSize = storage.getFileSize(_filename);
    Serial.printf("File size: %u bytes\n\n", (uint32_t)fileSize);

    if (fileSize == 0) {
        Serial.println("File is empty.");
    } else {
        String content;
        if (storage.readFile(_filename, content, READ_BUFFER_SIZE)) {
            Serial.println(content);
        } else {
            Serial.println("ERROR: Failed to read file!");
        }
    }

    Serial.println();
    Serial.printf("Free Heap after read: %d bytes\n", ESP.getFreeHeap());
    Serial.println("\nDone.");
    digitalWrite(LED_PIN, LOW);
}


void loop(){
}
