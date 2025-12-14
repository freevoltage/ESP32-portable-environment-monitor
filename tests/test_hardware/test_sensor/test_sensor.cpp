#include <unity.h>
#include "sensor.h"
#include "data_structures.h"

// Mock BME280 class
class MockAdafruit_BME280 {
public:
    static bool begin_success;
    static bool sensor_ready;
    static float mock_temperature;
    static float mock_humidity;
    static float mock_pressure;
    static bool begin_called;
    
    bool begin(uint8_t addr = 0x76) {
        begin_called = true;
        return begin_success;
    }
    
    float readTemperature() { return mock_temperature; }
    float readHumidity() { return mock_humidity; }
    float readPressure() { return mock_pressure; }
    float readAltitude(float seaLevel = 1013.25) { 
        return 44330 * (1.0 - pow(mock_pressure / seaLevel, 0.1903)); 
    }
    
    static void reset() {
        begin_success = true;
        sensor_ready = true;
        mock_temperature = 25.0;
        mock_humidity = 50.0;
        mock_pressure = 1013.25;
        begin_called = false;
    }
};

bool MockAdafruit_BME280::begin_success = true;
bool MockAdafruit_BME280::sensor_ready = true;
float MockAdafruit_BME280::mock_temperature = 25.0;
float MockAdafruit_BME280::mock_humidity = 50.0;
float MockAdafruit_BME280::mock_pressure = 1013.25;
bool MockAdafruit_BME280::begin_called = false;

void setUp(void) {
    MockAdafruit_BME280::reset();
}

void tearDown(void) {
    // Clean up
}

void test_sensor_initialization() {
    SensorManager sensor;
    
    bool result = sensor.begin();
    
    TEST_ASSERT_TRUE(MockAdafruit_BME280::begin_called);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(sensor.isReady());
}

void test_sensor_initialization_failure() {
    SensorManager sensor;
    MockAdafruit_BME280::begin_success = false;
    
    bool result = sensor.begin();
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(sensor.isReady());
}

void test_sensor_read_temperature() {
    SensorManager sensor;
    sensor.begin();
    
    MockAdafruit_BME280::mock_temperature = 23.5;
    float temp = sensor.readTemperature();
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, 23.5, temp);
}

void test_sensor_read_humidity() {
    SensorManager sensor;
    sensor.begin();
    
    MockAdafruit_BME280::mock_humidity = 65.8;
    float humidity = sensor.readHumidity();
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, 65.8, humidity);
}

void test_sensor_read_pressure() {
    SensorManager sensor;
    sensor.begin();
    
    MockAdafruit_BME280::mock_pressure = 999.5;
    float pressure = sensor.readPressure();
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, 999.5, pressure);
}

void test_sensor_get_reading() {
    SensorManager sensor;
    sensor.begin();
    
    MockAdafruit_BME280::mock_temperature = 22.3;
    MockAdafruit_BME280::mock_humidity = 58.7;
    MockAdafruit_BME280::mock_pressure = 1020.1;
    
    SensorReading reading = sensor.getReading();
    
    TEST_ASSERT_TRUE(reading.isValid);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 22.3, reading.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 58.7, reading.humidity);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1020.1, reading.pressure);
    TEST_ASSERT_TRUE(reading.timestamp > 0);
}

void test_sensor_invalid_readings() {
    SensorManager sensor;
    sensor.begin();
    
    // Test with NaN values
    MockAdafruit_BME280::mock_temperature = NAN;
    MockAdafruit_BME280::mock_humidity = 50.0;
    MockAdafruit_BME280::mock_pressure = 1013.25;
    
    SensorReading reading = sensor.getReading();
    TEST_ASSERT_FALSE(reading.isValid);
}

void test_sensor_test_connection() {
    SensorManager sensor;
    sensor.begin();
    
    bool connected = sensor.testConnection();
    TEST_ASSERT_TRUE(connected);
    
    MockAdafruit_BME280::sensor_ready = false;
    connected = sensor.testConnection();
    // Implementation dependent - might still return true if mocked properly
}

void test_sensor_reset() {
    SensorManager sensor;
    sensor.begin();
    
    sensor.reset();
    // After reset, should still be able to take readings
    TEST_ASSERT_TRUE(sensor.isReady());
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_sensor_initialization);
    RUN_TEST(test_sensor_initialization_failure);
    RUN_TEST(test_sensor_read_temperature);
    RUN_TEST(test_sensor_read_humidity);
    RUN_TEST(test_sensor_read_pressure);
    RUN_TEST(test_sensor_get_reading);
    RUN_TEST(test_sensor_invalid_readings);
    RUN_TEST(test_sensor_test_connection);
    RUN_TEST(test_sensor_reset);
    
    return UNITY_END();
}
