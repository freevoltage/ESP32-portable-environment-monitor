#include <unity.h>
#include <Arduino.h>
#include "data_service.h"
#include "mocks/mock_sensor_manager.h"
#include "mocks/mock_storage_manager.h" 
#include "mocks/mock_rtc_manager.h"

// Test fixtures
MockSensorManager* mockSensor;
MockStorageManager* mockStorage;
MockRTCManager* mockRTC;
DataService* dataService;

void setUp(void) {
    mockSensor = new MockSensorManager();
    mockStorage = new MockStorageManager();
    mockRTC = new MockRTCManager();
    dataService = new DataService(mockSensor, mockStorage, mockRTC);
}

void tearDown(void) {
    delete dataService;
    delete mockRTC;
    delete mockStorage;
    delete mockSensor;
}

// Test Cases
void test_collectCurrentReading_success() {
    // Arrange
    SensorReading expectedReading = {25.5, 60.2, 1013.25, 1234567890};
    mockSensor->setNextReading(expectedReading);
    
    // Act
    bool result = dataService->collectCurrentReading();
    
    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(dataService->hasCurrentReading());
    
    SensorReading actualReading = dataService->getCurrentReading();
    TEST_ASSERT_EQUAL_FLOAT(expectedReading.temperature, actualReading.temperature);
    TEST_ASSERT_EQUAL_FLOAT(expectedReading.humidity, actualReading.humidity);
}

void test_collectCurrentReading_sensor_failure() {
    // Arrange
    mockSensor->setShouldFail(true);
    
    // Act
    bool result = dataService->collectCurrentReading();
    
    // Assert
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(dataService->hasCurrentReading());
}

void test_storeCurrentReading_without_collecting_first() {
    // Act & Assert
    TEST_ASSERT_FALSE(dataService->storeCurrentReading());
}

void test_storeCurrentReading_success() {
    // Arrange
    SensorReading reading = {25.5, 60.2, 1013.25, 1234567890};
    mockSensor->setNextReading(reading);
    mockStorage->setShouldSucceed(true);
    
    dataService->collectCurrentReading();
    
    // Act
    bool result = dataService->storeCurrentReading();
    
    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mockStorage->wasStoreReadingCalled());
}


void setup() {
    delay(2000);
    UNITY_BEGIN();
    
    RUN_TEST(test_collectCurrentReading_success);
    RUN_TEST(test_collectCurrentReading_sensor_failure);
    RUN_TEST(test_storeCurrentReading_without_collecting_first);
    RUN_TEST(test_storeCurrentReading_success);
    
    UNITY_END();
}
