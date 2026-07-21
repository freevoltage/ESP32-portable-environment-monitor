#pragma once
#include "time_sync_service.h"
#include <cstdio>

static SyncMode mockTimeSyncMode = SyncMode::BLE_FIRST;
static SyncSource mockTimeSyncLastSource = SyncSource::NONE;
static time_t mockTimeSyncLastTime = 0;
static bool mockTimeSyncInProgress = false;
static bool mockTimeSyncShouldFail = false;
static bool mockBLESyncShouldFail = false;
static bool mockWiFiSyncShouldFail = false;
static bool mockTimeSyncInitialized = false;
static bool mockBLEActive = false;

TimeSyncService::TimeSyncService()
    : _mode(SyncMode::BLE_FIRST), _bleActive(false), _initialized(false),
      _pendingBLETime(0), _bleTimeReceived(false) {
    _status.mode = _mode;
}

TimeSyncService::~TimeSyncService() {
    stop();
}

bool TimeSyncService::begin(RTCManager* rtc, ConnectivityService* connectivity) {
    _mode = mockTimeSyncMode;
    _status.mode = _mode;
    _initialized = true;
    mockTimeSyncInitialized = true;
    return true;
}

void TimeSyncService::stop() {
    _bleActive = false;
    mockBLEActive = false;
    _initialized = false;
}

bool TimeSyncService::sync() {
    if (!_initialized) return false;

    mockTimeSyncInProgress = true;
    _status.syncInProgress = true;
    bool success = false;

    switch (_mode) {
        case SyncMode::OFF:
            _status.syncInProgress = false;
            mockTimeSyncInProgress = false;
            return false;

        case SyncMode::BLE_ONLY:
            success = syncBLE();
            break;

        case SyncMode::WIFI_ONLY:
            success = syncWiFi();
            break;

        case SyncMode::BLE_FIRST:
            success = syncBLE();
            if (!success) success = syncWiFi();
            break;

        case SyncMode::WIFI_FIRST:
            success = syncWiFi();
            if (!success) success = syncBLE();
            break;
    }

    _status.syncInProgress = false;
    mockTimeSyncInProgress = false;
    return success;
}

bool TimeSyncService::syncBLE() {
    if (mockBLESyncShouldFail || mockTimeSyncShouldFail) {
        setSyncResult(SyncSource::BLE, false);
        return false;
    }

    // Simulate receiving a time value
    _pendingBLETime = 1721566800; // Fixed test epoch
    _bleTimeReceived = true;

    setSyncResult(SyncSource::BLE, true);
    return true;
}

bool TimeSyncService::syncWiFi() {
    if (mockWiFiSyncShouldFail || mockTimeSyncShouldFail) {
        setSyncResult(SyncSource::WIFI, false);
        return false;
    }

    setSyncResult(SyncSource::WIFI, true);
    return true;
}

void TimeSyncService::startBLE() {
    _bleActive = true;
    mockBLEActive = true;
}

void TimeSyncService::stopBLE() {
    _bleActive = false;
    mockBLEActive = false;
}

bool TimeSyncService::isBLEAdvertising() const {
    return _bleActive;
}

void TimeSyncService::setMode(SyncMode mode) {
    _mode = mode;
    _status.mode = mode;
    mockTimeSyncMode = mode;
}

SyncMode TimeSyncService::getMode() const {
    return _mode;
}

const char* TimeSyncService::modeToString(SyncMode mode) {
    switch (mode) {
        case SyncMode::OFF:        return "OFF";
        case SyncMode::BLE_ONLY:   return "BLE";
        case SyncMode::WIFI_ONLY:  return "WiFi";
        case SyncMode::BLE_FIRST:  return "BLE+WiFi";
        case SyncMode::WIFI_FIRST: return "WiFi+BLE";
        default:                   return "???";
    }
}

SyncStatus TimeSyncService::getStatus() const {
    return _status;
}

SyncSource TimeSyncService::getLastSyncSource() const {
    return _status.lastSource;
}

time_t TimeSyncService::getLastSyncTime() const {
    return _status.lastSyncTime;
}

bool TimeSyncService::isSyncing() const {
    return _status.syncInProgress;
}

void TimeSyncService::setSyncResult(SyncSource source, bool success) {
    _status.lastSource = source;
    if (success) {
        _status.lastSyncTime = 1721566800; // Fixed test epoch
    }
    mockTimeSyncLastSource = source;
    mockTimeSyncLastTime = _status.lastSyncTime;
}

void TimeSyncService::saveConfig() {
    // No-op in mock
}

void TimeSyncService::loadConfig() {
    // No-op in mock — use default mode
}

extern "C" {
    void setMockTimeSyncMode(int mode) {
        mockTimeSyncMode = static_cast<SyncMode>(mode);
    }

    void setMockBLESyncShouldFail(bool shouldFail) {
        mockBLESyncShouldFail = shouldFail;
    }

    void setMockWiFiSyncShouldFail(bool shouldFail) {
        mockWiFiSyncShouldFail = shouldFail;
    }

    void setMockTimeSyncShouldFail(bool shouldFail) {
        mockTimeSyncShouldFail = shouldFail;
    }

    void resetMockTimeSync() {
        mockTimeSyncMode = SyncMode::BLE_FIRST;
        mockTimeSyncLastSource = SyncSource::NONE;
        mockTimeSyncLastTime = 0;
        mockTimeSyncInProgress = false;
        mockTimeSyncShouldFail = false;
        mockBLESyncShouldFail = false;
        mockWiFiSyncShouldFail = false;
        mockTimeSyncInitialized = false;
        mockBLEActive = false;
    }
}
