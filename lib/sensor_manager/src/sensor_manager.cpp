#include "sensor_manager.h"
#include <config.h>
#include <Adafruit_BME280.h>
#include <logger.h>

SensorManager::SensorManager() : initialized(false) {}

//SensorManager::SensorManager(uint8_t address) : i2cAddress(address), initialized(false) {}


bool SensorManager::begin() {
    Wire.begin(SDA, SCL);
    
    initialized = bme.begin(BME280_ADDRESS, &Wire);
    
    if (!initialized) {
        LOG_ERROR("Could not find BME280 sensor!");
        return false;
    }
    
    // Note: Might not be needed. We'll see.
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_X1,   // pressure
                    Adafruit_BME280::SAMPLING_X1,   // humidity
                    Adafruit_BME280::FILTER_OFF);

    LOG_INFO("BME280 sensor initialized successfully");
    return true;
}

bool SensorManager::isReady()
{
    return initialized;
    //return initialized && this->testConnection();
}

SensorReading SensorManager::getReading() {
    SensorReading reading;
    
    if(!initialized){
        Serial.println("Sensor not initialized");
        // reading struct has an internal valid which is false by default
        return reading; 
    }

    //time(&reading.timestamp);
    reading.temperature = readTemperature();
    reading.pressure = readPressure();
    reading.humidity = readHumidity();
    //reading.altitude = getAltitude(); // TODO 
    reading.timestamp = millis(); // TODO Verify that this is correct
    // How is this timestamp converted into a proper Date String when writing into memory
    // --> This is the responsibility of the Data Service
    
    validateReading(reading);

    if(reading.isValid){
        LOG_INFO("Sensor reading: %.1f°C, %.1f%%, %.1fhPa\n", 
                     reading.temperature, reading.humidity, reading.pressure);
    }
    else{
        LOG_INFO("Invalid sensor reading detected");

    }

    return reading;
}

float SensorManager::readTemperature() {
    if (!initialized) return NAN;
    return bme.readTemperature();
}

float SensorManager::readPressure() {
    if (!initialized) return NAN;
    return bme.readPressure() / 100.0F;
}

float SensorManager::readHumidity() {
    if (!initialized) return NAN;
    return bme.readHumidity();
}

float SensorManager::getAltitude() {
    if (!initialized) return NAN;
    return bme.readAltitude(SEALEVELPRESSURE_HPA);
}

bool SensorManager::testConnection() {
    if (!initialized) return false;
    
    // Try to read chip ID
    int sensorID = bme.sensorID();
    
    return (sensorID == 0x60);  // BME280 chip ID
}


// NOTE: I have no Idea why I would need that
void SensorManager::reset(){
    initialized = false;
    begin();
}

bool SensorManager::validateReading(SensorReading& reading){
    if (isnan(reading.temperature) || isnan(reading.humidity) || isnan(reading.pressure)){
        reading.isValid = false;
        return false;
    }

    if (reading.temperature < -40.0 || reading.temperature > 85.0){
        reading.isValid = false;
        return false;
    }

    if (reading.humidity < 0.0 ||reading.humidity > 100.0){
        reading.isValid = false;
        return false;
    }

    if (reading.pressure < 300.0 || reading.pressure > 1100.0){
        reading.isValid = false;
        return false;
    }
    reading.isValid = true;
    return true;
}

void SensorManager::printReading(const SensorReading &reading) {
    LOG_INFO("\n=== SENSOR READING ===");
    LOG_INFO("Unix Timestamp: %ld\n", reading.timestamp);
    LOG_INFO("Temperature: %.2f °C\n", reading.temperature);
    LOG_INFO("Humidity: %.2f %%\n", reading.humidity);
    LOG_INFO("Pressure: %.2f hPa\n", reading.pressure);
    LOG_INFO("=====================\n");
}