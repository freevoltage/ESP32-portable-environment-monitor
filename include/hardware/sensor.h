#pragma once
#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <time.h>
#include "data_structures.h"

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
    float getTemperature();
    float getHumidity();
    float getPressure();
    float getAltitude();
    
    //Utility
    bool testConnection();
    void reset();

    // Print reading to Serial
    void printReading(const SensorReading &reading);
    
    // Format reading as CSV line
    //String toCSV(const SensorReading &reading);
    
    // Get CSV header
    //String getCSVHeader();

    // TODO Implement some function to use the BME280 Temperature Compensation Method

private:
    Adafruit_BME280 bme;
    bool initialized;
    uint8_t i2cAddress;

    bool validateReading(SensorReading& reading);
};