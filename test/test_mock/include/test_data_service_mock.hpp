#include <unity.h>
#include "data_service.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "rtc_manager.h"
#include "logger.h"
#include "test_utils.h"
#include <test_fixture.h>

#include "mock_sensor_manager.h"
#include "mock_storage_manager.h"
#include "mock_rtc_manager.h"
#include "mock_control.h"

namespace test_data_service
{
    SensorManager *sensor = nullptr;
    StorageManager *storage = nullptr;
    RTCManager *rtc = nullptr;
    DataService *dataService = nullptr;

    const String TEST_FILE = "/test.csv";

    void test_initialization()
    {
        TEST_ASSERT_NOT_NULL(dataService);
        TEST_ASSERT_FALSE(dataService->hasCurrentReading());
    }

    void test_collect_current_reading()
    {
        bool success = dataService->collectCurrentReading();
        TEST_ASSERT_TRUE(success);
        TEST_ASSERT_TRUE(dataService->hasCurrentReading());

        SensorReading reading = dataService->getCurrentReading();
        TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading is not valid");
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, reading.timestamp, "Timestamp must be greater than zero");
    }

    void test_multiple_readings()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        SensorReading first = dataService->getCurrentReading();

        setMockEpoch(first.timestamp + 10);

        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        SensorReading second = dataService->getCurrentReading();

        TEST_ASSERT_NOT_EQUAL(first.timestamp, second.timestamp);
    }

    void test_store_current_reading()
    {
        LOG_INFO("Storage ready: %s", storage->isReady() ? "YES" : "NO");

        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        TEST_ASSERT_TRUE(dataService->storeCurrentReading());

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

        for (int i = 0; i < NUM_READINGS; i++)
        {
            setMockEpoch(1609459200 + (i * 10));
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
        }

        std::vector<SensorReading> all_readings;
        bool result = storage->getAllReadings(all_readings);
        printAllReadings("All Readings for Debugging", all_readings);

        std::vector<SensorReading> readings;
        TEST_ASSERT_TRUE(storage->getLastNReadings(readings, NUM_READINGS));
        TEST_ASSERT_EQUAL(NUM_READINGS, readings.size());
    }

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
        invalidReading.temperature = -100.0f;
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
        invalidReading.humidity = 150.0f;
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
        invalidReading.isValid = false;

        TEST_ASSERT_FALSE(dataService->isReadingValid(invalidReading));
    }

    void test_calculate_stats_no_data()
    {
        TemperatureStats stats = dataService->calculateStats(1);
        TEST_ASSERT_FALSE(stats.isValid);
        TEST_ASSERT_EQUAL(0, stats.sampleCount);
    }

    void test_calculate_stats_single_reading()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        SensorReading reading = dataService->getCurrentReading();
        TEST_ASSERT_TRUE(dataService->storeCurrentReading());

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

        for (int i = 0; i < NUM_READINGS; i++)
        {
            setMockEpoch(1609459200 + (i * 10));
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            expectedSum += dataService->getCurrentReading().temperature;
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
        }

        float expectedAvg = expectedSum / NUM_READINGS;

        TemperatureStats stats = dataService->calculateStats(1);

        TEST_ASSERT_TRUE(stats.isValid);
        TEST_ASSERT_EQUAL(NUM_READINGS, stats.sampleCount);
        assertFloatsEqual(expectedAvg, stats.average);
        TEST_ASSERT_LESS_OR_EQUAL(stats.max, stats.min);
    }

    void test_data_staleness_fresh()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        TEST_ASSERT_FALSE(dataService->isDataStale());
    }

    void test_data_staleness_old()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());

        SensorReading reading = dataService->getCurrentReading();
        setMockEpoch(reading.timestamp + 600);

        TEST_ASSERT_TRUE(dataService->isDataStale());
    }

    void test_time_since_last_reading_no_data()
    {
        unsigned long timeSince = dataService->getTimeSinceLastReading();
        TEST_ASSERT_EQUAL(0, timeSince);
    }

    void test_get_recent_readings_empty()
    {
        std::vector<SensorReading> readings = dataService->getRecentReadings(10);
        TEST_ASSERT_EQUAL(0, readings.size());
    }

    void test_get_recent_readings()
    {
        const int NUM_READINGS = 5;

        for (int i = 0; i < NUM_READINGS; i++)
        {
            setMockEpoch(1609459200 + (i * 10));
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
        }

        std::vector<SensorReading> readings = dataService->getRecentReadings(3);
        TEST_ASSERT_EQUAL(3, readings.size());
    }

    void test_get_recent_readings_request_more_than_available()
    {
        for (int i = 0; i < 3; i++)
        {
            setMockEpoch(1609459200 + (i * 10));
            TEST_ASSERT_TRUE(dataService->collectCurrentReading());
            TEST_ASSERT_TRUE(dataService->storeCurrentReading());
        }

        std::vector<SensorReading> readings = dataService->getRecentReadings(10);
        TEST_ASSERT_EQUAL(3, readings.size());
    }

    void test_collect_reading_sensor_error()
    {
        setMockSensorShouldFail(true);
        bool success = dataService->collectCurrentReading();
        TEST_ASSERT_FALSE(success);
        TEST_ASSERT_FALSE(dataService->hasCurrentReading());
        setMockSensorShouldFail(false);
    }

    void test_store_reading_without_collecting()
    {
        bool success = dataService->storeCurrentReading();
        TEST_ASSERT_FALSE(success);
    }

    void test_full_workflow()
    {
        TEST_ASSERT_TRUE(dataService->collectCurrentReading());
        TEST_ASSERT_TRUE(dataService->hasCurrentReading());

        TEST_ASSERT_TRUE(dataService->storeCurrentReading());

        TemperatureStats stats = dataService->calculateStats(1);
        TEST_ASSERT_TRUE(stats.isValid);

        std::vector<SensorReading> readings = dataService->getRecentReadings(1);
        TEST_ASSERT_EQUAL(1, readings.size());

        SensorReading current = dataService->getCurrentReading();
        assertFloatsEqual(current.temperature, readings[0].temperature);
        assertFloatsEqual(current.temperature, stats.average);
    }

    void setUp()
    {
        sensor = new SensorManager();
        storage = new StorageManager("/test.csv", 0);
        rtc = new RTCManager();

        sensor->begin();
        storage->begin();
        rtc->begin();

        setMockEpoch(1609459200);

        dataService = new DataService(sensor, storage, rtc);
        TEST_ASSERT_NOT_NULL(dataService);
    }

    void tearDown()
    {
        delete dataService;
        delete rtc;
        delete storage;
        delete sensor;

        dataService = nullptr;
        rtc = nullptr;
        storage = nullptr;
        sensor = nullptr;

        resetMockSensor();
        resetMockStorage();
        resetMockRtc();
    }

    void run_tests()
    {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        RUN_TEST(test_initialization);
        RUN_TEST(test_collect_current_reading);
        RUN_TEST(test_multiple_readings);

        RUN_TEST(test_store_current_reading);
        RUN_TEST(test_store_multiple_readings);

        RUN_TEST(test_reading_validation_valid);
        RUN_TEST(test_reading_validation_invalid_temperature);
        RUN_TEST(test_reading_validation_invalid_humidity);
        RUN_TEST(test_reading_validation_invalid_flag);

        RUN_TEST(test_calculate_stats_no_data);
        RUN_TEST(test_calculate_stats_single_reading);
        RUN_TEST(test_calculate_stats_multiple_readings);

        RUN_TEST(test_data_staleness_fresh);
        RUN_TEST(test_data_staleness_old);
        RUN_TEST(test_time_since_last_reading_no_data);

        RUN_TEST(test_get_recent_readings_empty);
        RUN_TEST(test_get_recent_readings);
        RUN_TEST(test_get_recent_readings_request_more_than_available);

        RUN_TEST(test_collect_reading_sensor_error);
        RUN_TEST(test_store_reading_without_collecting);

        RUN_TEST(test_full_workflow);

        TEST_CONTEXT.clearFixtures();
    }
}
