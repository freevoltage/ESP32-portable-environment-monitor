#include <Arduino.h>
#include <storage_manager.h>
#include <data_structures.h>
#include <config.h>

// TODO This Example would benefit from RTC Time Sync
// So that the written CSV Data is a little more realistic

#define LED_PIN LED_BUILTIN

// Forward Declarations
void testFileOperations();
void debugPrintSetup();

StorageManager storage;
SensorReading reading;

String _filename = "/datalog.csv";

void testBasicReadWrite();

void setup(){
    Serial.begin(115200);
    while(!Serial);
    delay(1000);

    Serial.println("=================================");
    Serial.println("ESP32 BME280 Storage Test Starting");
    Serial.println("=================================");

    debugPrintSetup();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    // Deselect Display CD Pin on shared SPI bus
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    // Initialize SD CArd
    Serial.println("Initializing SD Card...");

    if (!storage.begin()) {
        Serial.println("ERROR: SD card init failed!");
        while (true) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(250); // Fast blink = error
        }
    }
    Serial.println("SD Card initialized\n");

    // Delete the File just for Testing
    #ifdef DELETE_FILE
    Serial.println("DELETE FILE FROM SD CARD");
    storage.deleteFile(_filename);
    #endif

   // Test basic operations
    testBasicReadWrite();

    Serial.println("Setup complete - entering loop - Turn off LED in 3 seconds");
    delay(3000);
    digitalWrite(LED_PIN, LOW); // Turn off LED when done
    Serial.println("LED OFF");
}


void loop(){
}


void testBasicReadWrite() {
    // Write 3 readings
    Serial.println("Writing data...");

    time_t baseTime = 1704067200; // Jan 1, 2024 00:00:00

    storage.resetFile();

    for (int i = 0; i < 3; i++) {
        SensorReading reading(20.0 + i, 50.0 + i, 1000.0 + i, baseTime + (i*60));
        String timestamp = "2024-01-15 12:0" + String(i) + ":00";
        storage.storeReading(reading);
    }

    // Read and display
    Serial.println("\nFile content:");
    String content;
    if (storage.readFile(_filename, content, 1000)) {
        Serial.println(content);
    } else {
        Serial.println("Read failed!");
    }
}

/* 
void testFileOperations() {
    Serial.println("\n=== Testing File Operations ===");
    
    String filename = "/datalog.csv";
    if (storage.fileExists(filename)) {
        uint32_t size = storage.getFileSize(filename);
        Serial.printf("File exists: %s (Size: %d bytes)\n", filename.c_str(), size);
    } else {
        Serial.printf("File doesn't exist yet: %s\n", filename.c_str());
    }
    
    Serial.println("\n--- Writing Initial Test Data ---");
    for (int i = 0; i < 3; i++) {
        SensorReading testReading = SensorReading(23.0+i, 0, 0, 0);
        
        String testTime = "2024-01-15 10:0" + String(i) + ":00";
        
        if (storage.storeReading(testReading)) {
            Serial.printf("Test reading %d written\n", i + 1);
        } else {
            Serial.printf("Failed to write test reading %d\n", i + 1);
        }
    }

    String readBuffer;
    if(storage.readFile("/datalog.csv", readBuffer, 100)){
        Serial.println("File Content:");
        Serial.println(readBuffer);
    }
    else{
        Serial.println("File Read Failed!");
    }
} */

void debugPrintSetup() {
    Serial.println("Debug Info:");
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.println();
}
