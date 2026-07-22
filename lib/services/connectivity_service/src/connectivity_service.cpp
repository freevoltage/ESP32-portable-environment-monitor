#include "connectivity_service.h"
#include "logger.h"
#include "config.h"

#ifndef MOCK
#include <LittleFS.h>
#endif

ConnectivityService::ConnectivityService(WiFiManager* wifi, RTCManager* rtc)
    : wiFiManager(wifi), rtcManager(rtc),
      status(ConnectivityStatus::DISCONNECTED),
      lastSyncAttempt(0) {
}

bool ConnectivityService::connect(int timeoutMs) {
    if (!wiFiManager) {
        LOG_ERROR("WiFiManager not available");
        return false;
    }

    if (isConnected()) {
        LOG_INFO("Already connected");
        return true;
    }

    loadWiFiConfig();

    status = ConnectivityStatus::CONNECTING;
    LOG_INFO("Connecting to WiFi...");

    int timeoutSeconds = timeoutMs / 1000;
    bool connected = wiFiManager->connect(_wifiConfig.ssid, _wifiConfig.password, timeoutSeconds);

    if (connected) {
        status = ConnectivityStatus::CONNECTED;
        LOG_INFO("WiFi connected: %s", wiFiManager->getIPAddress().c_str());
    } else {
        status = ConnectivityStatus::DISCONNECTED;
        LOG_ERROR("WiFi connection failed");
    }

    return connected;
}

void ConnectivityService::disconnect() {
    if (wiFiManager) {
        wiFiManager->disconnect();
    }
    status = ConnectivityStatus::DISCONNECTED;
    LOG_INFO("Disconnected");
}

bool ConnectivityService::isConnected() const {
    return wiFiManager && wiFiManager->isConnected();
}

ConnectivityStatus ConnectivityService::getStatus() const {
    return status;
}

bool ConnectivityService::ensureTimeSync() {
    if (!rtcManager || !rtcManager->isReady()) {
        LOG_ERROR("RTC not available for time sync");
        return false;
    }

    // Connect if not already connected
    if (!isConnected()) {
        LOG_INFO("WiFi not connected, connecting first...");
        if (!connect()) {
            LOG_ERROR("Cannot sync time: WiFi connection failed");
            return false;
        }
    }

    status = ConnectivityStatus::TIME_SYNCING;
    LOG_INFO("Starting NTP time sync...");

    bool synced = wiFiManager->syncTimeNTP(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);

    if (synced) {
        // Update RTC with NTP time
        time_t now;
        time(&now);
        rtcManager->setTime(now);
        rtcManager->setLastSyncTime(now);
        lastSyncAttempt = millis();
        status = ConnectivityStatus::SYNC_COMPLETE;
        LOG_INFO("Time sync complete: %s", rtcManager->getFormattedTime().c_str());
    } else {
        status = ConnectivityStatus::CONNECTED;
        LOG_ERROR("NTP time sync failed");
    }

    return synced;
}

bool ConnectivityService::syncTimeIfNeeded() {
    if (isTimeSyncRequired()) {
        return ensureTimeSync();
    }
    LOG_DEBUG("Time sync not needed");
    return true;
}

bool ConnectivityService::isTimeSyncRequired() const {
    if (!rtcManager || !rtcManager->isReady()) {
        return true;
    }

    // Never synced before
    if (lastSyncAttempt == 0) {
        return true;
    }

    // Check interval
    return (millis() - lastSyncAttempt) >= SYNC_INTERVAL;
}

unsigned long ConnectivityService::getTimeSyncLastSync() const {
    return lastSyncAttempt;
}

// ---- WiFi Config ----

void ConnectivityService::loadWiFiConfig() {
    // Start LittleFS (idempotent — safe if already mounted)
    #ifndef MOCK
    if (!LittleFS.begin(true)) {
        LOG_ERROR("LittleFS mount failed for WiFi config");
    }
    #endif

    // Fall back to compile-time defaults
    strncpy(_wifiConfig.ssid, WIFI_DEFAULT_SSID, sizeof(_wifiConfig.ssid) - 1);
    strncpy(_wifiConfig.password, WIFI_DEFAULT_PASSWORD, sizeof(_wifiConfig.password) - 1);
    _wifiConfig.ssid[sizeof(_wifiConfig.ssid) - 1] = '\0';
    _wifiConfig.password[sizeof(_wifiConfig.password) - 1] = '\0';
    _wifiConfig.isValid = true;

    // Try to load from LittleFS
    #ifndef MOCK
    File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!file) {
        LOG_INFO("No WiFi config file, using defaults: %s", WIFI_DEFAULT_SSID);
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("ssid=")) {
            strncpy(_wifiConfig.ssid, line.substring(5).c_str(), sizeof(_wifiConfig.ssid) - 1);
            _wifiConfig.ssid[sizeof(_wifiConfig.ssid) - 1] = '\0';
        } else if (line.startsWith("password=")) {
            strncpy(_wifiConfig.password, line.substring(9).c_str(), sizeof(_wifiConfig.password) - 1);
            _wifiConfig.password[sizeof(_wifiConfig.password) - 1] = '\0';
        }
    }
    file.close();
    LOG_INFO("WiFi config loaded: %s", _wifiConfig.ssid);
    #endif
}

const WiFiConfig& ConnectivityService::getWiFiConfig() const {
    return _wifiConfig;
}
