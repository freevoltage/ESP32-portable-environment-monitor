// data_service.cpp
#include "data_service.h"
#include "logger.h"

DataService::DataService(SensorManager* sensor, StorageManager* storage, RTCManager* rtc)
    : sensorManager(sensor), storageManager(storage), rtcManager(rtc) {
    LOG_DEBUG("Allocate DataService object");
    _hasValidData = false;

    // Initialize with invalid Reading
    _lastReading = SensorReading();
}

bool DataService::collectCurrentReading() {
    if (!sensorManager || !sensorManager->isReady()) {
        LOG_ERROR("Sensor not available");
        return false;
    }
    
    // Get Readings from Sensor
    time_t timestamp = rtcManager->getEpochTime();
    _lastReading = sensorManager->getReading(timestamp);
    _hasValidData = isReadingValid(_lastReading);
    
    if (_hasValidData) {
        LOG_INFO("Valid reading collected");
    } else {
        LOG_WARN("Invalid reading collected");
    }

    LOG_INFO("Reading collected: T=%.1f°C, H=%.1f%%, P=%.1fhPa, Time=%ld", 
             _lastReading.temperature, _lastReading.humidity, _lastReading.pressure, _lastReading.timestamp);
    
    return _hasValidData;
}

SensorReading DataService::getCurrentReading() const {
    return _lastReading;
}

bool DataService::hasCurrentReading() const {
    return _hasValidData;
}

bool DataService::storeCurrentReading() {
    if (!_hasValidData) {
        LOG_ERROR("No valid reading to store");
        return false;
    }
    
    if (!storageManager || !storageManager->isReady()) {
        LOG_ERROR("Storage not available");
        return false;
    }
    
    // storeReading function will handle formatting automatically
    bool result = storageManager->storeReading(_lastReading);
    
    if (result) {
        LOG_DEBUG("Reading stored successfully");
    } else {
        LOG_ERROR("Failed to store reading");
    }

    return result;
}

TemperatureStats DataService::calculateStats(int hoursBack) {
    TemperatureStats stats;
    
    if (!storageManager || !storageManager->isReady()) {
        LOG_ERROR("Storage not available for stats");
        return stats;
    }
    
    if (!rtcManager || !rtcManager->isReady()) {
        LOG_ERROR("RTC not available for stats");
        return stats;
    }
    
    // Calculate cutoff time
    time_t currentTime = rtcManager->getEpochTime();
    time_t cutoffTime = currentTime - (hoursBack * 3600);
    
    LOG_DEBUG("Calculating stats for last %d hours (since %ld)", hoursBack, cutoffTime);

    std::vector<SensorReading> readings;
    if (!storageManager->getReadingsSince(cutoffTime, readings)) {
        LOG_ERROR("Failed to retrieve readings for stats");
        return stats;
    }

    if (readings.empty()) {
        LOG_WARN("No readings found in last %d hours", hoursBack);
        return stats;
    }

    return calculateStatsFromReadings(readings);
}

std::vector<SensorReading> DataService::getRecentReadings(int count) {
    std::vector<SensorReading> allReadings;
    std::vector<SensorReading> recentReadings;
    
    if (!storageManager || !storageManager->isReady()) {
        return recentReadings; // empty
    }
    
    // ✅ StorageManager handles parsing, we just get the results
    if (!storageManager->getAllReadings(allReadings)) {
        return recentReadings; // empty
    }
    
    // Take last 'count' readings
    int startIndex = std::max(0, (int)allReadings.size() - count);
    for (int i = startIndex; i < allReadings.size(); i++) {
        recentReadings.push_back(allReadings[i]);
    }
    
    return recentReadings;
}

bool DataService::isReadingValid(const SensorReading& reading) {
    if(!reading.isValid){
        LOG_DEBUG("Reading marked as invalid");
        return false;
    }
    
    // Validate temperature range (-40 to 85°C for BME280)
    if (reading.temperature < -40.0f || reading.temperature > 85.0f) {
        LOG_WARN("Temperature out of range: %.1f°C", reading.temperature);
        return false;
    }

    // Validate humidity range (0-100%)
    if (reading.humidity < 0.0f || reading.humidity > 100.0f) {
        LOG_WARN("Humidity out of range: %.1f%%", reading.humidity);
        return false;
    }

    // Validate pressure range (300-1100 hPa)
    if (reading.pressure < 300.0f || reading.pressure > 1100.0f) {
        LOG_WARN("Pressure out of range: %.1fhPa", reading.pressure);
        return false;
    }

    // Validate timestamp (must be reasonable)
    if (reading.timestamp < RTCManager::MIN_VALID_TIME) {
                LOG_WARN("Timestamp before NTP/BLE sync: %ld (sensor data still valid)", 
                 (long)reading.timestamp);
    }

    return true;
}

///
bool DataService::isDataStale() const {
    if (!_hasValidData) {
        return true;
    }
    

    if (!rtcManager || !rtcManager->isReady()) {
        return true;
    }

    time_t currentTime = rtcManager->getEpochTime();
    time_t readingAge = currentTime - _lastReading.timestamp;

    // Consider stale if older than 5 minutes
    const time_t STALE_THRESHOLD_SECONDS = 5 * 60;
    return readingAge > STALE_THRESHOLD_SECONDS;
}

unsigned long DataService::getTimeSinceLastReading() const {
    if (!_hasValidData) {
        return 0; // No Reading Available
    }
    
    if(!rtcManager || !rtcManager->isReady()){
        return 0;
    }

    time_t currentTime = rtcManager->getEpochTime();

    if(currentTime < _lastReading.timestamp){
        LOG_WARN("Current Time is smaller than reading timestamp!");
        return 0;
    }
    // Convert to millis
    return (currentTime - _lastReading.timestamp) * 1000UL;
}

// Private helper method - only does calculations, no parsing
TemperatureStats DataService::calculateStatsFromReadings(const std::vector<SensorReading>& readings) {
    TemperatureStats stats; // isValid = false by default
    
    if (readings.empty()) {
        LOG_DEBUG("Empty readings vector for stats");
        return stats;
    }

    // Initialize with first reading
    stats.min = readings[0].temperature;
    stats.max = readings[0].temperature;
    float tempSum = 0.0;

    // Calculate min, max, and sum
    for (const auto& reading : readings) {
        if (!reading.isValid) continue; // Skip invalid readings

        if (reading.temperature < stats.min) {
            stats.min = reading.temperature;
        }
        if (reading.temperature > stats.max) {
            stats.max = reading.temperature;
        }
        tempSum += reading.temperature;
    }

    // Calculate average
    stats.average = tempSum / readings.size();
    stats.sampleCount = readings.size();
    stats.isValid = true;

    LOG_INFO("Stats calculated: Min=%.1f, Max=%.1f, Avg=%.1f, Samples=%d",
             stats.min, stats.max, stats.average, stats.sampleCount);

    return stats;
}
