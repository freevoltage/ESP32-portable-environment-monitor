#pragma once
#include "storage_manager.h"
#include <cstdio>
#include <algorithm>

static std::vector<SensorReading> mockStorageVector;
static bool mockStorageInitialized = false;
static bool mockStorageShouldFail = false;

StorageManager::StorageManager(const String& filename, uint8_t csPin)
    : _filename(filename), _cs(csPin), _initialized(false), _sd_card_present(false) {}

bool StorageManager::begin() {
    if (mockStorageShouldFail) {
        _initialized = false;
        return false;
    }
    _initialized = true;
    _sd_card_present = true;
    mockStorageInitialized = true;
    return true;
}

bool StorageManager::isReady() {
    return _initialized && mockStorageInitialized;
}

void StorageManager::reset() {
    _initialized = false;
    _sd_card_present = false;
    mockStorageInitialized = false;
    mockStorageVector.clear();
}

bool StorageManager::storeReading(const SensorReading &reading) {
    if (!isReady()) return false;
    mockStorageVector.push_back(reading);
    return true;
}

bool StorageManager::appendLine(const String& csvLine) {
    if (!isReady()) return false;
    return true;
}

bool StorageManager::getAllReadings(std::vector<SensorReading> &readings) {
    if (!isReady()) return false;
    readings = mockStorageVector;
    return true;
}

bool StorageManager::getLastNReadings(std::vector<SensorReading> &readings, uint16_t maxCount) {
    if (!isReady()) return false;
    int start = std::max(0, (int)mockStorageVector.size() - (int)maxCount);
    readings.clear();
    for (int i = start; i < (int)mockStorageVector.size(); i++) {
        readings.push_back(mockStorageVector[i]);
    }
    return true;
}

bool StorageManager::getReadingsSince(time_t timestamp, std::vector<SensorReading> &readings, uint16_t maxCount) {
    if (!isReady()) return false;
    readings.clear();
    uint16_t count = 0;
    for (const auto& r : mockStorageVector) {
        if (r.timestamp >= timestamp && count < maxCount) {
            readings.push_back(r);
            count++;
        }
    }
    return true;
}

void StorageManager::cleanup() {
    // No-op in mock
}

bool StorageManager::fileExists(const String &filename) const {
    return mockStorageInitialized;
}

bool StorageManager::createFile(const String &filename) {
    return true;
}

bool StorageManager::deleteFile(const String &filename) {
    mockStorageVector.clear();
    return true;
}

bool StorageManager::clearFile() {
    mockStorageVector.clear();
    return true;
}

bool StorageManager::testConnection() {
    return isReady();
}

bool StorageManager::testSDCardHealth() {
    return isReady();
}

size_t StorageManager::getFileSize(const String &filename) const {
    return mockStorageVector.size() * sizeof(SensorReading);
}

size_t StorageManager::getUsedSpace() {
    return mockStorageVector.size() * sizeof(SensorReading);
}

void StorageManager::printCardInfo() {
    printf("MOCK SD: %zu readings stored\n", mockStorageVector.size());
}

void StorageManager::listFiles() {
    printf("MOCK - datalog.csv (%zu bytes)\n", mockStorageVector.size() * sizeof(SensorReading));
}

bool StorageManager::readFile(const String &filename, String &buff, size_t maxSize) {
    buff = "";
    for (const auto& r : mockStorageVector) {
        char line[64];
        snprintf(line, sizeof(line), "%.1f,%.1f,%.1f,%ld,%d",
                 r.temperature, r.humidity, r.pressure, (long)r.timestamp, r.isValid ? 1 : 0);
        buff += String(line) + "\n";
        if (buff.size() > maxSize) break;
    }
    return true;
}

uint16_t StorageManager::estimateLineCount() {
    return mockStorageVector.size();
}

extern "C" {
    void resetMockStorage() {
        mockStorageVector.clear();
        mockStorageInitialized = false;
        mockStorageShouldFail = false;
    }
}
