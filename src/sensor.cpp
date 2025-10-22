#include "sensor.h"
#include "config.h"
#include <Wire.h>

SensorManager::SensorManager() : sensorAvailable(false) {}

bool SensorManager::begin() {
    Wire.begin(SDA, SCL);
    
    sensorAvailable = bme.begin(BME280_ADDRESS, &Wire);
    
    if (!sensorAvailable) {
        Serial.println("ERROR: Could not find BME280 sensor!");
        return false;
    }
    
    Serial.println("BME280 sensor initialized successfully");
    return true;
}

SensorReading SensorManager::takeReading() {
    SensorReading reading;
    
    time(&reading.timestamp);
    reading.temperature = getTemperature();
    reading.pressure = getPressure();
    reading.humidity = getHumidity();
    reading.altitude = getAltitude();
    
    return reading;
}

float SensorManager::getTemperature() {
    if (!sensorAvailable) return 0.0;
    return bme.readTemperature();
}

float SensorManager::getPressure() {
    if (!sensorAvailable) return 0.0;
    return bme.readPressure() / 100.0F;
}

float SensorManager::getHumidity() {
    if (!sensorAvailable) return 0.0;
    return bme.readHumidity();
}

float SensorManager::getAltitude() {
    if (!sensorAvailable) return 0.0;
    return bme.readAltitude(SEALEVELPRESSURE_HPA);
}

void SensorManager::printReading(const SensorReading &reading) {
    Serial.println("\n=== SENSOR READING ===");
    Serial.printf("Unix Timestamp: %ld\n", reading.timestamp);
    Serial.printf("Temperature: %.2f °C\n", reading.temperature);
    Serial.printf("Pressure: %.2f hPa\n", reading.pressure);
    Serial.printf("Humidity: %.2f %%\n", reading.humidity);
    Serial.printf("Altitude: %.2f m\n", reading.altitude);
    Serial.println("=====================\n");
}

String SensorManager::toCSV(const SensorReading &reading) {
    String csv = "";
    csv += String(reading.timestamp) + ",";
    csv += String(reading.temperature, 2) + ",";
    csv += String(reading.pressure, 2) + ",";
    csv += String(reading.humidity, 2) + ",";
    csv += String(reading.altitude, 2);
    return csv;
}

String SensorManager::getCSVHeader() {
    return "Timestamp,Temperature_C,Pressure_hPa,Humidity_%,Altitude_m";
}
