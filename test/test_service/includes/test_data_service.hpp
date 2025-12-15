#include <unity.h>
#include <Arduino.h>
#include "data_service.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "rtc_manager.h"

#include "logger.h"
#include "test_utils.h"

namespace test_data_service
{
    // Test fixtures
    SensorManager* sensor = nullptr;
    StorageManager* storage = nullptr;
    RTCManager* rtc = nullptr;
    DataService* dataService = nullptr;

    // Test file name
    //const String TEST_FILE = "test_data.csv"; // TODO MY File Management is messed up a little and I think I can not use another filename than the standard one currently. I need to refactor the code to enable it. For now I keep the standard file name.
    const String TEST_FILE = SD_FILENAME;

    void init_test_fixtures()
    {
        LOG_DEBUG("Initialize test fixture before test.");
        // Initialize components
        sensor = new SensorManager();
        TEST_ASSERT_NOT_NULL_MESSAGE(sensor, "Failed to allocate SensorManager");

        storage = new StorageManager();
        TEST_ASSERT_NOT_NULL_MESSAGE(storage, "Failed to allocate StorageManager");

        rtc = new RTCManager();
        TEST_ASSERT_NOT_NULL_MESSAGE(rtc, "Failed to allocate RTCManager");


        // Initialize hardware
        bool result;
        result = sensor->begin();
        TEST_ASSERT_TRUE(result);

        result = storage->begin();
        TEST_ASSERT_TRUE(result);

        result = rtc->begin();
        TEST_ASSERT_TRUE(result);

        // Set a known time for testing
        // TODO This Formatting Things should be inside the RTC Test
        rtc->setTime(1609459200);
        //LOG_INFO("RTC getFormattedTime() = %s", rtc->getFormattedTime().c_str());
        //LOG_INFO("RTC getTime() = %s", rtc->getTime());
        //LOG_INFO("RTC getEpochTime() = %lu", rtc->getEpochTime());

        TEST_ASSERT_TRUE(storage->testSDCardHealth());

        // Create DataService
        dataService = new DataService(sensor, storage, rtc);
        
        // Clean up any existing test data
        if (storage->fileExists(TEST_FILE)) {
            LOG_DEBUG("File '%s' already exist. Will be deleted for test.", TEST_FILE);
            storage->deleteFile(TEST_FILE);
        }
        LOG_INFO("Test Fixture initialized");
    }

    void cleanup_test_fixtures()
    {
        LOG_DEBUG("Cleanup test fixture");
        // Cleanup
        if (storage && storage->isReady() && storage->fileExists(TEST_FILE)) {
            LOG_DEBUG("Delete file before next test.");
            TEST_ASSERT_TRUE(storage->deleteFile(TEST_FILE));
            delay(50);
            TEST_ASSERT_FALSE(storage->fileExists(TEST_FILE));
        }
        
        if(storage && storage->isReady()){
            SD.end();
            delay(50);
        }

        delete dataService;
        delete rtc;
        delete storage;
        delete sensor;
        
        dataService = nullptr;
        rtc = nullptr;
        storage = nullptr;
        sensor = nullptr;

        LOG_INFO("Test fixture cleaned up");
    }

    // ========================================
    // BASIC FUNCTIONALITY TESTS
    // ========================================

    void test_initialization()
    {
        TEST_ASSERT_NOT_NULL(dataService);
        //TEST_ASSERT_FALSE(dataService->hasCurrentReading());
    }

    void test_collect_current_reading()
    {   
        bool success = dataService->collectCurrentReading();
        
        TEST_ASSERT_TRUE(success);
        TEST_ASSERT_TRUE(dataService->hasCurrentReading());
        
        SensorReading reading = dataService->getCurrentReading();
        TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading is not valid");
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, reading.timestamp, "Timestemp must be greater than zero");
    }

    void test_multiple_readings()
    {
        // Collect first reading
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        SensorReading first = dataService->getCurrentReading();
        
        delay(1000); // Small delay
        
        // Collect second reading
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        SensorReading second = dataService->getCurrentReading();
        
        // Timestamps should be different)
        TEST_ASSERT_NOT_EQUAL(first.timestamp, second.timestamp);
    }

    // ========================================
    // STORAGE INTEGRATION TESTS
    // ========================================

    void test_store_current_reading()
    {
        LOG_INFO("Storage ready: %s", storage->isReady() ? "YES" : "NO");
        LOG_INFO("File '%s' exists: %s", TEST_FILE, storage->fileExists(TEST_FILE) ? "YES (false)" : "NO (correct)");

        // Collect and store a reading
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        TEST_ASSERT_TRUE(dataService->storeCurrentReading());

        LOG_INFO("File '%s' exists after write: %s", TEST_FILE, storage->fileExists(TEST_FILE) ? "YES" : "NO");

        // Try to read the file directly
        File testFile = SD.open(SD_FILENAME, FILE_READ);
        if (testFile) {
            LOG_INFO("File size: %lu bytes. Estimated lines: %u", testFile.size()), storage->estimateLineCount();
            testFile.close();
        } else {
            LOG_ERROR("Cannot open file for verification!");
        }
        
        // Verify it was stored
        std::vector<SensorReading> readings;
        TEST_ASSERT_TRUE(storage->getLastNReadings(readings, 1));
        TEST_ASSERT_EQUAL(1, readings.size());

        SensorReading stored = readings[0];
        SensorReading current = dataService->getCurrentReading();
        
        assertReadingsEqual(current, stored);
    }

    void test_store_multiple_readings()
    {
        const int NUM_READINGS = 5;
        
        for (int i = 0; i < NUM_READINGS; i++) {
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
            delay(100);
        }
        
        std::vector<SensorReading> all_readings;
        bool result = storage->getAllReadings(all_readings);
        printAllReadings("All Readings for Debugging", all_readings);

        std::vector<SensorReading> readings;
        TEST_ASSERT_TRUE(storage->getLastNReadings(readings, NUM_READINGS));
        TEST_ASSERT_EQUAL(NUM_READINGS, readings.size());
    }

    // ========================================
    // DATA VALIDATION TESTS
    // ========================================

    void test_reading_validation_valid()
    {
        SensorReading validReading;
        validReading.temperature = 25.0f;
        validReading.humidity = 50.0f;
        validReading.pressure = 1013.25f;
        validReading.timestamp = rtc->getEpochTime();
        validReading.isValid = true;
        
        TEST_ASSERT_TRUE(dataService->isReadingValid(validReading));
    }

    void test_reading_validation_invalid_temperature()
    {
        SensorReading invalidReading;
        invalidReading.temperature = -100.0f; // Unrealistic
        invalidReading.humidity = 50.0f;
        invalidReading.pressure = 1013.25f;
        invalidReading.timestamp = rtc->getEpochTime();
        invalidReading.isValid = true;
        
        TEST_ASSERT_FALSE(dataService->isReadingValid(invalidReading));
    }

    void test_reading_validation_invalid_humidity()
    {
        SensorReading invalidReading;
        invalidReading.temperature = 25.0f;
        invalidReading.humidity = 150.0f; // > 100%
        invalidReading.pressure = 1013.25f;
        invalidReading.timestamp = rtc->getEpochTime();
        invalidReading.isValid = true;
        
        TEST_ASSERT_FALSE(dataService->isReadingValid(invalidReading));
    }

    void test_reading_validation_invalid_flag()
    {
        SensorReading invalidReading;
        invalidReading.temperature = 25.0f;
        invalidReading.humidity = 50.0f;
        invalidReading.pressure = 1013.25f;
        invalidReading.timestamp = rtc->getEpochTime();
        invalidReading.isValid = false; // Marked invalid
        
        TEST_ASSERT_FALSE(dataService->isReadingValid(invalidReading));
    }

    // ========================================
    // STATISTICS TESTS
    // ========================================

    void test_calculate_stats_no_data()
    {
        TemperatureStats stats = dataService->calculateStats(1);
        
        TEST_ASSERT_FALSE(stats.isValid);
        TEST_ASSERT_EQUAL(0, stats.sampleCount);
    }

    void test_calculate_stats_single_reading()
    {
        // Store one reading
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        SensorReading reading = dataService->getCurrentReading();
        TEST_ASSERT_TRUE(dataService->storeCurrentReading());
        
        // Calculate stats
        TemperatureStats stats = dataService->calculateStats(1);
        
        TEST_ASSERT_TRUE(stats.isValid);
        TEST_ASSERT_EQUAL(1, stats.sampleCount);
        
        assertFloatsEqual(reading.temperature, stats.min);
        assertFloatsEqual(reading.temperature, stats.max);
        assertFloatsEqual(reading.temperature, stats.average);
    }

    void test_calculate_stats_multiple_readings()
    {
        const int NUM_READINGS = 10;
        float expectedSum = 0;
        
        // Store multiple readings
        for (int i = 0; i < NUM_READINGS; i++) {
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            expectedSum += dataService->getCurrentReading().temperature;
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
            delay(100);
        }
        
        float expectedAvg = expectedSum / NUM_READINGS;
        
        // Calculate stats
        TemperatureStats stats = dataService->calculateStats(1);
        
        TEST_ASSERT_TRUE(stats.isValid);
        TEST_ASSERT_EQUAL(NUM_READINGS, stats.sampleCount);
        assertFloatsEqual(expectedAvg, stats.average);
        //TEST_ASSERT_FLOAT_WITHIN(0.1, expectedAvg, stats.average);
        TEST_ASSERT_LESS_OR_EQUAL(stats.max, stats.min); // min <= max
    }

    // ========================================
    // STALENESS TESTS
    // ========================================

    void test_data_staleness_fresh()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        
        TEST_ASSERT_FALSE(dataService->isDataStale());
        TEST_ASSERT_LESS_THAN(1000, dataService->getTimeSinceLastReading());
    }

    void test_data_staleness_old()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        
        // Simulate old data by waiting
        delay(2000);
        
        unsigned long timeSince = dataService->getTimeSinceLastReading();
        TEST_ASSERT_GREATER_OR_EQUAL(2000, timeSince);
    }

    void test_time_since_last_reading_no_data()
    {
        unsigned long timeSince = dataService->getTimeSinceLastReading();
        
        // Should return 0
        TEST_ASSERT_EQUAL(0, timeSince);
    }

    // ========================================
    // RECENT READINGS TESTS
    // ========================================

    void test_get_recent_readings_empty()
    {
        std::vector<SensorReading> readings = dataService->getRecentReadings(10);
        
        TEST_ASSERT_EQUAL(0, readings.size());
    }

    void test_get_recent_readings()
    {
        const int NUM_READINGS = 5;
        
        // Store some readings
        for (int i = 0; i < NUM_READINGS; i++) {
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
            delay(100);
        }
        
        // Get recent readings
        std::vector<SensorReading> readings = dataService->getRecentReadings(3);
        
        TEST_ASSERT_EQUAL(3, readings.size());
        
        // Verify they're in reverse chronological order (most recent first)
        for (size_t i = 1; i < readings.size(); i++) {
            TEST_ASSERT_GREATER_OR_EQUAL(readings[i].timestamp, readings[i-1].timestamp);
        }
    }

    void test_get_recent_readings_request_more_than_available()
    {
        // Store 3 readings
        for (int i = 0; i < 3; i++) {
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
            delay(100);
        }
        
        // Request 10, should get 3
        std::vector<SensorReading> readings = dataService->getRecentReadings(10);
        
        TEST_ASSERT_EQUAL(3, readings.size());
    }

    // ========================================
    // ERROR HANDLING TESTS
    // ========================================

    void test_collect_reading_sensor_error()
    {
        // This test requires a way to simulate sensor failure
        // For now, just verify it handles the error gracefully
        
        // Note: Actual implementation depends on SensorManager behavior
        bool success = dataService->collectCurrentReading();
        
        // Should either succeed or fail gracefully
        if (!success) {
            TEST_ASSERT_FALSE(dataService->hasCurrentReading());
        }
    }

    void test_store_reading_without_collecting()
    {
        // Try to store without collecting first
        bool success = dataService->storeCurrentReading();
        
        // Should fail gracefully
        TEST_ASSERT_FALSE(success);
    }

    // ========================================
    // INTEGRATION TESTS
    // ========================================

    void test_full_workflow()
    {
        // 1. Collect reading
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        TEST_ASSERT_TRUE(dataService->hasCurrentReading());
        
        // 2. Store reading
        TEST_ASSERT_TRUE(dataService->storeCurrentReading());
        
        // 3. Calculate stats
        TemperatureStats stats = dataService->calculateStats(1);
        TEST_ASSERT_TRUE(stats.isValid);
        
        // 4. Get recent readings
        std::vector<SensorReading> readings = dataService->getRecentReadings(1);
        TEST_ASSERT_EQUAL(1, readings.size());
        
        // 5. Verify data consistency
        SensorReading current = dataService->getCurrentReading();
        assertFloatsEqual(current.temperature, readings[0].temperature);
        assertFloatsEqual(current.temperature, stats.average);
    }

    // ========================================
    // TEST RUNNER
    // ========================================

    void run_tests()
    {
        UnitySetTestFile(__FILE__);
        
        // Basic functionality
        RUN_TEST(test_initialization);
        RUN_TEST(test_collect_current_reading);
        RUN_TEST(test_multiple_readings);
        
        // Storage integration
        RUN_TEST(test_store_current_reading);
        RUN_TEST(test_store_multiple_readings); //! Recently Failed in [storeReading] with Failed to open file for writing
        
        // Validation
        RUN_TEST(test_reading_validation_valid);
        RUN_TEST(test_reading_validation_invalid_temperature);
        RUN_TEST(test_reading_validation_invalid_humidity);
        RUN_TEST(test_reading_validation_invalid_flag);
        
        // Statistics
        RUN_TEST(test_calculate_stats_no_data);
        RUN_TEST(test_calculate_stats_single_reading);
        RUN_TEST(test_calculate_stats_multiple_readings);
        
        // Staleness
        RUN_TEST(test_data_staleness_fresh);
        RUN_TEST(test_data_staleness_old);
        RUN_TEST(test_time_since_last_reading_no_data); // This test fails
        
        // Recent readings
        RUN_TEST(test_get_recent_readings_empty);
        RUN_TEST(test_get_recent_readings);
        RUN_TEST(test_get_recent_readings_request_more_than_available);
        
        // Error handling
        RUN_TEST(test_collect_reading_sensor_error);
        RUN_TEST(test_store_reading_without_collecting);
        
        // Integration
        RUN_TEST(test_full_workflow);
    }
}


void setUp() {
    LOG_DEBUG("Test Data Service SetUp");
    test_data_service::init_test_fixtures();
}

void tearDown() {
    LOG_DEBUG("Test Data Service TearDown");
    test_data_service::cleanup_test_fixtures();
}