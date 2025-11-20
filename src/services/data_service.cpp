// data_service.cpp
#include "services/data_service.h"

DataService::DataService(SensorManager* sensor, StorageManager* storage, RTCManager* rtc)
    : sensorManager(sensor), storageManager(storage), rtcManager(rtc) {
    hasValidData = false;
}

bool DataService::collectCurrentReading() {
    if (!sensorManager || !sensorManager->isReady()) {
        Serial.println("Sensor not available");
        return false;
    }
    
    lastReading = sensorManager->getReading();
    hasValidData = isReadingValid(lastReading);
    
    if (hasValidData) {
        Serial.println("Successfully collected sensor reading");
    } else {
        Serial.println("Invalid reading collected");
    }
    
    return hasValidData;
}

SensorReading DataService::getCurrentReading() const {
    return lastReading;
}

bool DataService::hasCurrentReading() const {
    return hasValidData;
}

bool DataService::storeCurrentReading() {
    if (!hasValidData) {
        Serial.println("No valid reading to store");
        return false;
    }
    
    if (!storageManager || !storageManager->isReady()) {
        Serial.println("Storage not available");
        return false;
    }
    
    if (!rtcManager || !rtcManager->isInitialized()) {
        Serial.println("RTC not available");
        return false;
    }
    
    // Get current datetime string from RTC
    String dateTime = DateTimeUtils::formatDateTime(rtcManager->getEpochTime());
    
    return storageManager->storeReading(lastReading, dateTime);
}

TemperatureStats DataService::calculateStats(int hoursBack) {
    TemperatureStats stats;
    
    if (!storageManager || !storageManager->isReady()) {
        Serial.println("Storage not available");
        return stats;
    }
    
    if (!rtcManager || !rtcManager->isInitialized()) {
        Serial.println("RTC not available");
        return stats;
    }
    
    // Calculate cutoff time
    time_t currentTime = rtcManager->getEpochTime();
    time_t cutoffTime = currentTime - (hoursBack * 3600);
    
    // ✅ Get parsed readings from StorageManager (StorageManager does ALL parsing)
    std::vector<SensorReading> readings;
    if (!storageManager->getReadingsSince(cutoffTime, readings)) {
        Serial.println("Failed to retrieve readings");
        return stats;
    }
    
    // ✅ Just do calculations on the parsed data
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
    return reading.isValid;
}

bool DataService::isDataStale() const {
    if (!hasValidData) {
        return true;
    }
    
    // Consider data stale if it's older than 5 minutes
    const unsigned long STALE_THRESHOLD_MS = 5 * 60 * 1000;
    return (millis() - lastReading.timestamp) > STALE_THRESHOLD_MS;
}

unsigned long DataService::getTimeSinceLastReading() const {
    if (!hasValidData) {
        return 0;
    }
    
    return millis() - lastReading.timestamp;
}

// ✅ Private helper method - only does calculations, no parsing
TemperatureStats DataService::calculateStatsFromReadings(const std::vector<SensorReading>& readings) {
    TemperatureStats stats; // isValid = false by default
    
    if (readings.empty()) {
        Serial.println("No readings available for stats calculation");
        return stats;
    }
    
    float tempSum = 0.0;
    stats.min = readings[0].temperature;
    stats.max = readings[0].temperature;
    
    for (const auto& reading : readings) {
        if (reading.temperature < stats.min) {
            stats.min = reading.temperature;
        }
        if (reading.temperature > stats.max) {
            stats.max = reading.temperature;
        }
        tempSum += reading.temperature;
    }
    
    stats.average = tempSum / readings.size();
    stats.sampleCount = readings.size();
    stats.isValid = true;
    
    Serial.printf("Calculated stats: Min=%.1f, Max=%.1f, Avg=%.1f, Count=%d\n",
                  stats.min, stats.max, stats.average, stats.sampleCount);
    
    return stats;
}
