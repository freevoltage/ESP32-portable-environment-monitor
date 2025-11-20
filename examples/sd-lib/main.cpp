#include <Arduino.h>
#include "hardware/storage.h"
#include "hardware/sensor.h"
#include "data_structures.h"
#include "config.h"

// TODO This Example would benefit from RTC Time Sync
// So that the written CSV Data is a little more realistic

#define LED_PIN LED_BUILTIN

// Forward Declarations
void testFileOperations();
void debugPrintSetup();

StorageManager storage;
SensorReading reading;

String _filename = "/datalog.csv";

void setup(){
    Serial.begin(115200);
    while(!Serial);

    delay(3000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESP32 BME280 Storage Test Starting");
    Serial.println("=================================");
    Serial.flush(); // Force output

    debugPrintSetup();
    // TODO The
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    Serial.print("LED BUILTIN PIN: ");
    Serial.println(LED_PIN);
    
    // You must not forget to explicitly Deselect the Displays Chip Select
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    Serial.println("Hardware pins configured");
    Serial.flush();
    delay(1000);

    Serial.println("Initializing SD Card...");
    Serial.flush();

    bool result;
    result = storage.begin();
    if(!result){
        Serial.println("SD Card Initialization failed (EXAMPLE)");
        while(true){
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(250); // Fast blink = error
        }
    }
    Serial.println("SD Card initialized.. EXAMPLE START");
    Serial.println("---- List Files ----");

    // Delete the File just for Testing
    #ifdef DELETE_FILE
    Serial.println("DELETE FILE FROM SD CARD");
    storage.deleteFile(_filename);
    #endif

    storage.listFiles();

    testFileOperations();
    Serial.println("Setup complete - entering loop - Turn off LED in 3 seconds");
    delay(3000);
    digitalWrite(LED_PIN, LOW); // Turn off LED when done
    Serial.println("LED OFF");
}


void loop(){
}

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
        
        if (storage.storeReading(testReading, testTime)) {
            Serial.printf("Test reading %d written\n", i + 1);
        } else {
            Serial.printf("Failed to write test reading %d\n", i + 1);
        }
    }


    String readBuffer;
    if(storage.readFile("/datalog.csv", readBuffer)){
        Serial.println("File Content:");
        Serial.println(readBuffer);
    }
    else{
        Serial.println("File Read Failed!");
    }
}

void debugPrintSetup() {
    Serial.println("Debug Info:");
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.println();
}
