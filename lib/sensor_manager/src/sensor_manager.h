#pragma once

#include <time.h>
#include <data_structures.h>

#ifndef MOCK
#include <Adafruit_BME280.h>
#endif

class SensorManager {
public:
    SensorManager();

    //SensorManager(uint8_t address = 0x76);
    
    // Initialize the BME280 sensor
    bool begin();
    bool isReady();
    
    // Take a complete sensor reading with timestamp
    SensorReading getReading();
    
    // Get individual readings
    float readTemperature();
    float readHumidity();
    float readPressure();
    float getAltitude(); // TODO Implement function getAltitude
    
    //Utility
    bool testConnection();
    void reset();

    // Print reading to Serial
    void printReading(const SensorReading &reading);

    // TODO Implement some function to use the BME280 Temperature Compensation Method

private:
#ifndef MOCK
    Adafruit_BME280 bme;
#endif
    bool initialized;
    uint8_t i2cAddress;

    bool validateReading(SensorReading& reading);
};