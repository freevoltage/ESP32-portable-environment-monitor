#include <Arduino.h>
#include <unity.h>
#include "sensor.h"
#include "../test_helpers.h"

SensorManager sensor;
SensorReading lastReading;

void test_sensor_initialization() {
    TestHelper::printTestHeader("Sensor Initialization");
    
    bool initialized = sensor.begin();
    
    if (!initialized) {
        TestHelper::printFail("BME280 not found - check wiring");
        TEST_FAIL_MESSAGE("Sensor not found");
        return;
    }
    
    TEST_ASSERT_TRUE(initialized);
    TestHelper::printPass("BME280 initialized successfully");
}

void test_sensor_reading() {
    TestHelper::printTestHeader("Sensor Reading");
    
    lastReading = sensor.takeReading();
    
    TestHelper::printSeparator();
    sensor.printReading(lastReading);
    TestHelper::printSeparator();
    
    // Just verify we got some data
    TEST_ASSERT_NOT_EQUAL(0.0, lastReading.temperature);
    
    TestHelper::printPass("Reading obtained");
}

void test_sensor_temperature_range() {
    TestHelper::printTestHeader("Temperature Range Check");
    
    SensorReading reading = sensor.takeReading();
    
    // BME280 valid range: -40°C to +85°C
    bool valid = TestHelper::assertInRange(
        reading.temperature, -40.0, 85.0, "Temperature"
    );
    
    TEST_ASSERT_TRUE(valid);
    TestHelper::printPass("Temperature in valid range");
}

void test_sensor_humidity_range() {
    TestHelper::printTestHeader("Humidity Range Check");
    
    SensorReading reading = sensor.takeReading();
    
    // BME280 valid range: 0% to 100%
    bool valid = TestHelper::assertInRange(
        reading.humidity, 0.0, 100.0, "Humidity"
    );
    
    TEST_ASSERT_TRUE(valid);
    TestHelper::printPass("Humidity in valid range");
}

void test_sensor_pressure_range() {
    TestHelper::printTestHeader("Pressure Range Check");
    
    SensorReading reading = sensor.takeReading();
    
    // BME280 valid range: 300 to 1100 hPa
    bool valid = TestHelper::assertInRange(
        reading.pressure, 300.0, 1100.0, "Pressure"
    );
    
    TEST_ASSERT_TRUE(valid);
    TestHelper::printPass("Pressure in valid range");
}

void test_sensor_consistency() {
    TestHelper::printTestHeader("Reading Consistency");
    
    SensorReading reading1 = sensor.takeReading();
    delay(1000);
    SensorReading reading2 = sensor.takeReading();
    
    float tempDiff = abs(reading1.temperature - reading2.temperature);
    float humidityDiff = abs(reading1.humidity - reading2.humidity);
    float pressureDiff = abs(reading1.pressure - reading2.pressure);
    
    Serial.printf("Temperature difference: %.2f°C\n", tempDiff);
    Serial.printf("Humidity difference: %.2f%%\n", humidityDiff);
    Serial.printf("Pressure difference: %.2f hPa\n", pressureDiff);
    
    // Consecutive readings should be similar
    TEST_ASSERT_LESS_THAN(5.0, tempDiff);
    TEST_ASSERT_LESS_THAN(10.0, humidityDiff);
    TEST_ASSERT_LESS_THAN(10.0, pressureDiff);
    
    TestHelper::printPass("Readings are consistent");
}

void setup() {
    Serial.begin(115200);
    TestHelper::waitForSerial();
    
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_sensor_initialization);
    RUN_TEST(test_sensor_reading);
    RUN_TEST(test_sensor_temperature_range);
    RUN_TEST(test_sensor_humidity_range);
    RUN_TEST(test_sensor_pressure_range);
    RUN_TEST(test_sensor_consistency);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
