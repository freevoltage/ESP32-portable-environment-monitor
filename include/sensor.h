#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <time.h>

struct SensorReading {
    time_t timestamp;
    float temperature;
    float pressure;
    float humidity;
    float altitude;
};

class SensorManager {
public:
    SensorManager();
    
    // Initialize the BME280 sensor
    bool begin();
    
    // Take a complete sensor reading with timestamp
    SensorReading takeReading();
    
    // Get individual readings
    float getTemperature();
    float getPressure();
    float getHumidity();
    float getAltitude();
    
    // Print reading to Serial
    void printReading(const SensorReading &reading);
    
    // Format reading as CSV line
    String toCSV(const SensorReading &reading);
    
    // Get CSV header
    String getCSVHeader();

private:
    Adafruit_BME280 bme;
    bool sensorAvailable;
};

#endif
