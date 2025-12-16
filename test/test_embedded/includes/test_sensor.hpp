#pragma once

#include <Arduino.h>
#include <unity.h>
#include "test_runner.h"
#include <test_fixture.h>
#include "sensor_manager.h"
#include "data_structures.h"
#include <logger.h>

SensorManager testSensor;

void test_sensor_initialization() {
    bool result = testSensor.begin();
    TEST_ASSERT_TRUE_MESSAGE(result, "Sensor initialization should succeed");
    TEST_ASSERT_TRUE_MESSAGE(testSensor.isReady(), "Sensor should be ready after initialization");
}

void test_sensor_connection() {
    bool connected = testSensor.testConnection();
    TEST_ASSERT_TRUE_MESSAGE(connected, "Sensor connection test should pass");
}

void test_sensor_individual_readings() {
    float temperature = testSensor.readTemperature();
    TEST_ASSERT_TRUE_MESSAGE(temperature > -50.0f && temperature < 100.0f, "Temperature should be in reasonable range");

    float humidity = testSensor.readHumidity();
    TEST_ASSERT_TRUE_MESSAGE(humidity >= 0.0f && humidity <= 100.0f, "Humidity should be in valid range");

    float pressure = testSensor.readPressure();
    TEST_ASSERT_TRUE_MESSAGE(pressure > 800.0f && pressure < 1200.0f, "Pressure should be in reasonable range");
}

void test_sensor_complete_reading() {
    SensorReading reading = testSensor.getReading();
    TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading should be valid");
    TEST_ASSERT_TRUE_MESSAGE(reading.temperature > -50.0f && reading.temperature < 100.0f, "Temperature should be in reasonable range");
    TEST_ASSERT_TRUE_MESSAGE(reading.humidity >= 0.0f && reading.humidity <= 100.0f, "Humidity should be in valid range");
    TEST_ASSERT_TRUE_MESSAGE(reading.pressure > 800.0f && reading.pressure < 1200.0f, "Pressure should be in reasonable range");
    TEST_ASSERT_TRUE_MESSAGE(reading.timestamp > 0, "Timestamp should be set");
}

void test_sensor_altitude() {
    float altitude = testSensor.getAltitude();
    TEST_ASSERT_TRUE_MESSAGE(altitude > -500.0f && altitude < 10000.0f, "Altitude should be in reasonable range");
}

void test_sensor_reset() {
    testSensor.reset();
    TEST_ASSERT_TRUE_MESSAGE(testSensor.isReady(), "Sensor should still be ready after reset");
    
    // Verify sensor still works after reset
    SensorReading reading = testSensor.getReading();
    TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading should still be valid after reset");
}

namespace test_sensor {

    void setUp(){}

    void tearDown(){}

    void run_tests() {
        UnitySetTestFile(__FILE__);

        LOG_INFO("\n[RUN TEST] SENSOR:\n");
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        RUN_TEST(test_sensor_initialization);
        RUN_TEST(test_sensor_connection);
        RUN_TEST(test_sensor_individual_readings);
        RUN_TEST(test_sensor_complete_reading);
        RUN_TEST(test_sensor_altitude);
        RUN_TEST(test_sensor_reset);

        TEST_CONTEXT.clearFixtures();
    }
}
