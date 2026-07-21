#pragma once
#include "rtc_manager.h"
#include <cstdio>

static time_t mockEpochTime = 1609459200; // 2021-01-01 00:00:00 UTC
static bool mockRtcInitialized = false;

RTCManager::RTCManager() : _lastSyncTime(0), _initialized(false) {}

bool RTCManager::begin() {
    _initialized = true;
    mockRtcInitialized = true;
    return true;
}

bool RTCManager::isReady() const {
    return _initialized;
}

bool RTCManager::setTime(int year, int month, int day, int hour, int min, int sec) {
    if (!_initialized) return false;
    return true;
}

bool RTCManager::setTime(unsigned long epoch, int ms) {
    if (!_initialized) return false;
    mockEpochTime = (time_t)epoch;
    return true;
}

time_t RTCManager::getEpochTime() {
    if (!_initialized) return 0;
    return mockEpochTime;
}

String RTCManager::getTime() {
    if (!_initialized) return String();
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long)mockEpochTime);
    return String(buf);
}

String RTCManager::getFormattedTime(const String& format) {
    if (!_initialized) return "Not initialized";
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long)mockEpochTime);
    return String(buf);
}

tm RTCManager::getTimeStruct() {
    tm emptyStruct = {};
    if (!_initialized) return emptyStruct;
    time_t t = mockEpochTime;
    gmtime_r(&t, &emptyStruct);
    return emptyStruct;
}

void RTCManager::setLastSyncTime(time_t syncTime) {
    _lastSyncTime = syncTime;
}

time_t RTCManager::getLastSyncTime() const {
    return _lastSyncTime;
}

bool RTCManager::needsTimeSync(int defaultIntervalHours) {
    if (!_initialized || _lastSyncTime == 0) return true;
    time_t now = getEpochTime();
    time_t intervalSeconds = defaultIntervalHours * 3600;
    return (now - _lastSyncTime) >= intervalSeconds;
}

bool RTCManager::isTimeSet() {
    if (!_initialized) return false;
    return getEpochTime() >= MIN_VALID_TIME;
}

void RTCManager::printRTCFormats() {
    printf("MOCK RTC - Epoch: %ld\n", (long)mockEpochTime);
}

extern "C" {
    void setMockEpoch(time_t epoch) {
        mockEpochTime = epoch;
    }

    void resetMockRtc() {
        mockEpochTime = 1609459200;
        mockRtcInitialized = false;
    }
}
