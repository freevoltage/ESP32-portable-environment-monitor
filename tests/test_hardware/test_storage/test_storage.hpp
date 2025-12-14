#include <Arduino.h>
#include <unity.h>
#include "test_runner.h"
#include <storage.h>
#include <data_structures.h>

StorageManager testStorage;
String testFilename = "/test_data.csv";

void test_storage_initialization() {
    bool result = testStorage.begin();
    TEST_ASSERT_TRUE_MESSAGE(result, "Storage initialization should succeed");
    TEST_ASSERT_TRUE_MESSAGE(testStorage.isReady(), "Storage should be ready after initialization");
}

void test_storage_file_operations() {
    // Clean up any existing test file
    if (testStorage.fileExists(testFilename)) {
        testStorage.deleteFile(testFilename);
    }
    
    // Test file creation and writing
    String testData = "timestamp,temperature,humidity,pressure\n";
    testData += "1234567890,25.5,60.0,1013.25\n";
    
    bool writeResult = testStorage.writeFile(testFilename, testData);
    TEST_ASSERT_TRUE_MESSAGE(writeResult, "Should be able to write test file");
    
    // Test file exists
    bool exists = testStorage.fileExists(testFilename);
    TEST_ASSERT_TRUE_MESSAGE(exists, "Test file should exist after writing");
    
    // Test file reading
    String readData = testStorage.readFile(testFilename);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(testData.c_str(), readData.c_str(), "Read data should match written data");
    
    // Clean up
    testStorage.deleteFile(testFilename);
}

void test_storage_sensor_data_logging() {
    // Clean up
    if (testStorage.fileExists(testFilename)) {
        testStorage.deleteFile(testFilename);
    }
    
    // Create test sensor reading
    SensorReading reading;
    reading.temperature = 22.5;
    reading.humidity = 55.0;
    reading.pressure = 1015.3;
    reading.isValid = true;
    reading.timestamp = 1234567890;
    
    // Test data logging
    bool logResult = testStorage.logSensorData(reading, testFilename);
    TEST_ASSERT_TRUE_MESSAGE(logResult, "Should be able to log sensor data");
    
    // Verify data was written
    String loggedData = testStorage.readFile(testFilename);
    TEST_ASSERT_TRUE_MESSAGE(loggedData.length() > 0, "Logged data should not be empty");
    TEST_ASSERT_TRUE_MESSAGE(loggedData.indexOf("22.5") != -1, "Should contain temperature value");
    TEST_ASSERT_TRUE_MESSAGE(loggedData.indexOf("55.0") != -1, "Should contain humidity value");
    TEST_ASSERT_TRUE_MESSAGE(loggedData.indexOf("1015.3") != -1, "Should contain pressure value");
    
    // Clean up
    testStorage.deleteFile(testFilename);
}

void test_storage_error_conditions() {
    // Test operations when storage is not ready (simulate failure)
    StorageManager failStorage;
    // Don't call begin() to simulate failure
    
    TEST_ASSERT_FALSE_MESSAGE(failStorage.isReady(), "Storage should not be ready without initialization");
    
    SensorReading reading = {25.0, 60.0, 1013.0, true, 1234567890};
    bool result = failStorage.logSensorData(reading, "/test.csv");
    TEST_ASSERT_FALSE_MESSAGE(result, "Should fail to log data when storage not ready");
}

namespace test_storage {
    void run_tests() {
        UnitySetTestFile(__FILE__);
        RUN_TEST(test_storage_initialization);
        RUN_TEST(test_storage_file_operations);
        RUN_TEST(test_storage_sensor_data_logging);
        RUN_TEST(test_storage_error_conditions);
    }
}
