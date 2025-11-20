#include "hardware/sensor.h"
#include "config.h" // TODO Dependency is only needed for SEALEVELPRESSURE_HPA. You should remove it.

SensorManager::SensorManager() : initialized(false) {}

//SensorManager::SensorManager(uint8_t address) : i2cAddress(address), initialized(false) {}


bool SensorManager::begin() {
    Wire.begin(SDA, SCL);
    
    initialized = bme.begin(BME280_ADDRESS, &Wire);
    
    if (!initialized) {
        Serial.println("ERROR: Could not find BME280 sensor!");
        return false;
    }
    
    // Note: Might not be needed. We'll see.
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_X1,   // pressure
                    Adafruit_BME280::SAMPLING_X1,   // humidity
                    Adafruit_BME280::FILTER_OFF);

    Serial.println("BME280 sensor initialized successfully");
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
    reading.temperature = getTemperature();
    reading.pressure = getPressure();
    reading.humidity = getHumidity();
    //reading.altitude = getAltitude(); // TODO 
    reading.timestamp = millis(); // TODO Verify that this is correct
    // How is this timestamp converted into a proper Date String when writing into memory
    // --> This is the responsibility of the Data Service
    
    validateReading(reading);

    if(reading.isValid){
        Serial.printf("Sensor reading: %.1f°C, %.1f%%, %.1fhPa\n", 
                     reading.temperature, reading.humidity, reading.pressure);
    }
    else{
        Serial.println("Invalid sensor reading detected");

    }

    return reading;
}

float SensorManager::getTemperature() {
    if (!initialized) return NAN;
    return bme.readTemperature();
}

float SensorManager::getPressure() {
    if (!initialized) return NAN;
    return bme.readPressure() / 100.0F;
}

float SensorManager::getHumidity() {
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
    Wire.beginTransmission(i2cAddress);
    Wire.write(0xD0); // BME280 chip ID register
    if (Wire.endTransmission() != 0) return false;
    
    Wire.requestFrom(i2cAddress, (uint8_t)1);
    if (Wire.available()) {
        uint8_t chipId = Wire.read();
        return (chipId == 0x60); // BME280 chip ID
    }
    
    return false;
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
    Serial.println("\n=== SENSOR READING ===");
    Serial.printf("Unix Timestamp: %ld\n", reading.timestamp);
    Serial.printf("Temperature: %.2f °C\n", reading.temperature);
    Serial.printf("Humidity: %.2f %%\n", reading.humidity);
    Serial.printf("Pressure: %.2f hPa\n", reading.pressure);
    //Serial.printf("Altitude: %.2f m\n", reading.altitude);
    Serial.println("=====================\n");
}

// TODO The responsibilty for this is in the DataService Class
/*
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
*/