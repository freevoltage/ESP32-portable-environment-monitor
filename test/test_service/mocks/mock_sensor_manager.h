#include "sensor_manager.h"
#include "includes/mock_sensor_helper.h"
#include <math.h>

// Mock data storage (not in header - test-only)
static SensorReading mockReading = {25.0f, 60.0f, 1013.25f, 0};
static bool mockInitialized = false;
static bool mockShouldFail = false;
static bool mockConnectionOK = true;

// Constructor
SensorManager::SensorManager() : initialized(false), i2cAddress(0x76) {
    // Mock constructor - no hardware initialization needed
}

// Mock implementations
bool SensorManager::begin() {
    if (mockShouldFail) {
        initialized = false;
        return false;
    }
    
    initialized = true;
    mockInitialized = true;
    return true;
}

bool SensorManager::isReady() {
    return initialized && mockInitialized && mockConnectionOK;
}

SensorReading SensorManager::getReading() {
    if (!isReady()) {
        return {NAN, NAN, NAN, millis()};
    }
    
    mockReading.timestamp = millis();
    return mockReading;
}

float SensorManager::readTemperature() {
    return isReady() ? mockReading.temperature : NAN;
}

float SensorManager::readHumidity() {
    return isReady() ? mockReading.humidity : NAN;
}

float SensorManager::readPressure() {
    return isReady() ? mockReading.pressure : NAN;
}

float SensorManager::getAltitude() {
    if (!isReady()) return NAN;
    // Mock altitude calculation
    // TODO Is this correct?
    return 44330.0 * (1.0 - pow(mockReading.pressure / 1013.25, 0.1903));
}

bool SensorManager::testConnection() {
    return mockConnectionOK;
}

void SensorManager::reset() {
    initialized = false;
    mockInitialized = false;
    mockReading = {25.0f, 60.0f, 1013.25f, 0};
}

void SensorManager::printReading(const SensorReading &reading) {
    Serial.printf("MOCK - Temp: %.2f°C, Humidity: %.2f%%, Pressure: %.2f hPa\n",
                  reading.temperature, reading.humidity, reading.pressure);
}

bool SensorManager::validateReading(SensorReading& reading) {
    if (isnan(reading.temperature) || isnan(reading.humidity) || isnan(reading.pressure)) {
        reading.isValid = false;
        return false;
    }
    
    // Basic range validation
    if (reading.temperature < -40.0 || reading.temperature > 85.0 ||
        reading.humidity < 0.0 || reading.humidity > 100.0 ||
        reading.pressure < 300.0 || reading.pressure > 1100.0) {
        reading.isValid = false;
        return false;
    }
    
    reading.isValid = true;
    return true;
}

// =============================================================================
// MOCK CONTROL FUNCTIONS (for tests only)
// =============================================================================

extern "C" {
    void setMockSensorReading(float temp, float humidity, float pressure) {
        mockReading = {temp, humidity, pressure, millis()};
    }
    
    void setMockSensorShouldFail(bool shouldFail) {
        mockShouldFail = shouldFail;
    }
    
    void setMockConnectionOK(bool connectionOK) {
        mockConnectionOK = connectionOK;
    }
    
    void resetMockSensor() {
        mockReading = {25.0f, 60.0f, 1013.25f, 0};
        mockShouldFail = false;
        mockConnectionOK = true;
        mockInitialized = false;
    }
}
