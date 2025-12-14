#define MOCK
#include <unity.h>
#include <storage_manager.h>
#include <data_structures.h>
#include "include/mock_sd.h"

StorageManager* storage;

void setUp() {
    SD.clearAll();
    storage = new StorageManager(5);
    storage->begin();
}

void tearDown() {
    delete storage;
}

// Helper to create test reading
SensorReading createReading(float temp, float hum, float press, time_t ts) {
    SensorReading r;
    r.temperature = temp;
    r.humidity = hum;
    r.pressure = press;
    r.timestamp = ts;
    r.isValid = true;
    return r;
}

void test_store_single_reading() {
    SensorReading reading = createReading(25.5f, 60.0f, 1013.25f, 1704067200);
    
    bool result = storage->storeReading(reading);
    
    TEST_ASSERT_TRUE(result);
    
    std::string content = SD.getFileContent("/datalog.csv");
    TEST_ASSERT_TRUE(content.find("25.5") != std::string::npos);
    TEST_ASSERT_TRUE(content.find("60.0") != std::string::npos);
}

void test_store_multiple_readings() {
    storage->storeReading(createReading(20.0f, 50.0f, 1013.0f, 1704067200));
    storage->storeReading(createReading(21.0f, 51.0f, 1013.1f, 1704067260));
    storage->storeReading(createReading(22.0f, 52.0f, 1013.2f, 1704067320));
    
    std::string content = SD.getFileContent("/datalog.csv");
    
    // Count lines (should be 3 data lines + 1 header)
    int lineCount = 0;
    for (char c : content) {
        if (c == '\n') lineCount++;
    }
    
    TEST_ASSERT_EQUAL(4, lineCount);
}

void test_read_all_readings() {
    // Store known data
    storage->storeReading(createReading(20.0f, 50.0f, 1013.0f, 1704067200));
    storage->storeReading(createReading(21.0f, 51.0f, 1013.1f, 1704067260));
    
    std::vector<SensorReading> readings;
    bool success = storage->getAllReadings(readings);
    
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(2, readings.size());
    TEST_ASSERT_EQUAL_FLOAT(20.0f, readings[0].temperature);
    TEST_ASSERT_EQUAL_FLOAT(21.0f, readings[1].temperature);
}

void test_read_last_n_readings() {
    // Store 5 readings
    for (int i = 0; i < 5; i++) {
        storage->storeReading(createReading(20.0f + i, 50.0f, 1013.0f, 1704067200 + i * 60));
    }
    
    std::vector<SensorReading> readings;
    bool success = storage->getLastNReadings(readings, 3);
    
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(3, readings.size());
    TEST_ASSERT_EQUAL_FLOAT(22.0f, readings[0].temperature); // Should be last 3
    TEST_ASSERT_EQUAL_FLOAT(23.0f, readings[1].temperature);
    TEST_ASSERT_EQUAL_FLOAT(24.0f, readings[2].temperature);
}

void test_empty_file_reading() {
    std::vector<SensorReading> readings;
    bool success = storage->getAllReadings(readings);
    
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, readings.size());
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_store_single_reading);
    RUN_TEST(test_store_multiple_readings);
    RUN_TEST(test_read_all_readings);
    RUN_TEST(test_read_last_n_readings);
    RUN_TEST(test_empty_file_reading);
    
    return UNITY_END();
}
