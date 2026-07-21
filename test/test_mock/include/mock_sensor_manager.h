#pragma once
#include "sensor_manager.h"
#include <cstdio>

static SensorReading mockSensorReading = SensorReading(25.0f, 60.0f, 1013.25f, 0);
static bool mockSensorInitialized = false;
static bool mockSensorShouldFail = false;
static bool mockSensorConnectionOK = true;

SensorManager::SensorManager() : initialized(false), i2cAddress(0x76) {}

bool SensorManager::begin() {
    if (mockSensorShouldFail) {
        initialized = false;
        return false;
    }
    initialized = true;
    mockSensorInitialized = true;
    return true;
}

bool SensorManager::isReady() {
    return initialized && mockSensorInitialized && mockSensorConnectionOK && !mockSensorShouldFail;
}

SensorReading SensorManager::getReading(time_t timestamp) {
    SensorReading reading;
    if (!isReady()) {
        reading.isValid = false;
        reading.timestamp = timestamp;
        return reading;
    }
    reading = mockSensorReading;
    reading.altitude = getAltitude();
    reading.timestamp = timestamp;
    reading.isValid = true;
    return reading;
}

SensorReading SensorManager::getReading() {
    return getReading(0);
}

float SensorManager::readTemperature() {
    return isReady() ? mockSensorReading.temperature : NAN;
}

float SensorManager::readHumidity() {
    return isReady() ? mockSensorReading.humidity : NAN;
}

float SensorManager::readPressure() {
    return isReady() ? mockSensorReading.pressure : NAN;
}

float SensorManager::getAltitude() {
    if (!isReady()) return NAN;
    return 44330.0 * (1.0 - pow(mockSensorReading.pressure / 1013.25, 0.1903));
}

bool SensorManager::testConnection() {
    return mockSensorConnectionOK;
}

void SensorManager::reset() {
    initialized = false;
    mockSensorInitialized = false;
        mockSensorReading = SensorReading(25.0f, 60.0f, 1013.25f, 0);
}

void SensorManager::printReading(const SensorReading &reading) {
    printf("MOCK - Temp: %.2f C, Humidity: %.2f%%, Pressure: %.2f hPa\n",
           reading.temperature, reading.humidity, reading.pressure);
}

bool SensorManager::validateReading(SensorReading& reading) {
    if (isnan(reading.temperature) || isnan(reading.humidity) || isnan(reading.pressure)) {
        reading.isValid = false;
        return false;
    }
    if (reading.temperature < -40.0 || reading.temperature > 85.0 ||
        reading.humidity < 0.0 || reading.humidity > 100.0 ||
        reading.pressure < 300.0 || reading.pressure > 1100.0) {
        reading.isValid = false;
        return false;
    }
    reading.isValid = true;
    return true;
}

extern "C" {
    void setMockSensorReading(float temp, float humidity, float pressure) {
        mockSensorReading = SensorReading(temp, humidity, pressure, 0);
    }

    void setMockSensorShouldFail(bool shouldFail) {
        mockSensorShouldFail = shouldFail;
    }

    void setMockConnectionOK(bool connectionOK) {
        mockSensorConnectionOK = connectionOK;
    }

    void resetMockSensor() {
        mockSensorReading = SensorReading(25.0f, 60.0f, 1013.25f, 0);
        mockSensorShouldFail = false;
        mockSensorConnectionOK = true;
        mockSensorInitialized = false;
    }
}
