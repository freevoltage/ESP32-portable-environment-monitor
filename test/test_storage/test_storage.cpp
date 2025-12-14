#include <Arduino.h>
#include <unity.h>

#include "hardware/storage.h"
#include "hardware/sensor.h"
#include "data_structures.h"
#include "config.h"

// I need to include my own test runner because the unity test runner does not work transmit the test result correctly to pio.
#include "test_runner.h" 

StorageManager storage;
SensorReading reading;

String _filename = "/datalog.csv";


void setUp(){
    if (!storage.isReady()) {
        bool result = storage.begin();
        TEST_ASSERT_TRUE_MESSAGE(result, "SD Card initialization failed");
    }
}

void tearDown(){
}


void test_storage_initialization(void) {
    bool isReady = storage.isReady();
    TEST_ASSERT_TRUE_MESSAGE(isReady, "Storage should be ready after initialization");
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





void setup(){
    Serial.begin(115200);
    while(!Serial);

    delay(3000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESP32 BME280 Storage Test Starting");
    Serial.println("=================================");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("LED BUILTIN PIN: ");
    Serial.println(LED_BUILTIN);
    
    // You must not forget to explicitly Deselect the Displays Chip Select
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    Serial.println("Hardware pins configured");
    delay(1000);

    Serial.println("Initializing SD Card...");

    bool result;
    result = storage.begin();
    
    RUN_TEST(test_storage_initialization);

    Serial.println("SD Card initialized.. EXAMPLE START");
    Serial.println("---- List Files ----");


    storage.listFiles();

    testFileOperations();
    Serial.println("Setup complete - entering loop - Turn off LED in 3 seconds");
    delay(3000);
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED when done
    Serial.println("LED OFF");
}

void loop(){
    delay(1000);
}
