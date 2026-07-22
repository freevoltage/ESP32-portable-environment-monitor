#ifndef MOCK
#include "time_sync_service.h"
#include <config.h>
#include <rtc_manager.h>
#include <connectivity_service.h>
#include <logger.h>
#include <LittleFS.h>

// NimBLE includes
#include <NimBLEDevice.h>

// Custom UUIDs for the Hiking Station Time Sync service
// Service: 4c54494d-6568-6b69-6e67-53746174696f
static const char* SERVICE_UUID        = "4c54494d-6568-6b69-6e67-53746174696f";
// Characteristic for writing time from phone
static const char* TIME_WRITE_UUID     = "4c54494d-7469-6d65-7772-697465000001";
// Characteristic for reading device time
static const char* TIME_READ_UUID      = "4c54494d-7469-6d65-7772-697465000002";

// NimBLE callback: receives time write from phone
class TimeWriteCallback : public NimBLECharacteristicCallbacks {
public:
    TimeSyncService* service;

    TimeWriteCallback(TimeSyncService* svc) : service(svc) {}

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 4) {
            uint32_t epoch;
            memcpy(&epoch, value.data(), 4);
            LOG_INFO("BLE time write: %lu", (unsigned long)epoch);
            service->_pendingBLETime = epoch;
            service->_bleTimeReceived = true;
        }
    }
};

// NimBLE callback: handles server events
class TimeServerCallback : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        LOG_INFO("BLE client connected");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        LOG_INFO("BLE client disconnected");
        // Restart advertising after disconnect
        pServer->getAdvertising()->start();
    }
};

// Global callbacks
static TimeSyncService* g_timeSyncInstance = nullptr;
static TimeWriteCallback* g_writeCallback = nullptr;
static TimeServerCallback* g_serverCallback = nullptr;

// ---- Constructor / Destructor ----

TimeSyncService::TimeSyncService()
    : _rtc(nullptr), _connectivity(nullptr),
      _mode(SyncMode::BLE_FIRST), _bleActive(false), _initialized(false),
      _pendingBLETime(0), _bleTimeReceived(false) {
    _status.mode = _mode;
}

TimeSyncService::~TimeSyncService() {
    stop();
}

// ---- Initialization ----

bool TimeSyncService::begin(RTCManager* rtc, ConnectivityService* connectivity) {
    LOG_INFO("TimeSyncService begin()");

    _rtc = rtc;
    _connectivity = connectivity;

    // Initialize LittleFS for config persistence
    if (!LittleFS.begin(true)) {
        LOG_ERROR("LittleFS mount failed");
    }

    loadConfig();
    _initialized = true;

    LOG_INFO("TimeSyncService initialized, mode=%s", modeToString(_mode));
    return true;
}

void TimeSyncService::stop() {
    stopBLE();
    _initialized = false;
}

// ---- Sync Operations ----

bool TimeSyncService::sync(ProgressCallback progress) {
    if (!_initialized) {
        LOG_ERROR("TimeSyncService not initialized");
        return false;
    }

    _status.syncInProgress = true;
    bool success = false;

    switch (_mode) {
        case SyncMode::OFF:
            LOG_INFO("Sync disabled (mode=OFF)");
            _status.syncInProgress = false;
            return false;

        case SyncMode::BLE_ONLY:
            success = syncBLE(progress);
            if (!success) {
                LOG_WARN("BLE sync failed, mode is BLE_ONLY so no fallback");
            }
            break;

        case SyncMode::WIFI_ONLY:
            success = syncWiFi();
            if (!success) {
                LOG_WARN("WiFi sync failed, mode is WIFI_ONLY so no fallback");
            }
            break;

        case SyncMode::BLE_FIRST:
            success = syncBLE(progress);
            if (!success) {
                LOG_INFO("BLE failed, falling back to WiFi");
                if (progress) progress("WiFi fallback...");
                success = syncWiFi();
            }
            break;

        case SyncMode::WIFI_FIRST:
            success = syncWiFi();
            if (!success) {
                LOG_INFO("WiFi failed, falling back to BLE");
                success = syncBLE(progress);
            }
            break;
    }

    _status.syncInProgress = false;
    return success;
}

bool TimeSyncService::syncBLE(ProgressCallback progress) {
    LOG_INFO("Starting BLE time sync...");

    _bleTimeReceived = false;
    _pendingBLETime = 0;

    startBLE();

    // Wait for phone to write time
    unsigned long start = millis();
    int lastReported = -1;
    while (!_bleTimeReceived && (millis() - start < BLE_SYNC_TIMEOUT_MS)) {
        // Show countdown every second
        if (progress) {
            int elapsed = (millis() - start) / 1000;
            if (elapsed != lastReported) {
                lastReported = elapsed;
                int remaining = (BLE_SYNC_TIMEOUT_MS / 1000) - elapsed;
                char buf[32];
                snprintf(buf, sizeof(buf), "BLE %ds...", remaining);
                progress(buf);
            }
        }
        delay(100);
    }

    stopBLE();

    if (_bleTimeReceived && _pendingBLETime > 0) {
        // Apply the received time to RTC
        if (_rtc) {
            _rtc->setTime(_pendingBLETime);
        }
        setSyncResult(SyncSource::BLE, true);
        LOG_INFO("BLE sync success: epoch %lu", (unsigned long)_pendingBLETime);
        return true;
    }

    LOG_WARN("BLE sync timed out");
    setSyncResult(SyncSource::BLE, false);
    return false;
}

bool TimeSyncService::syncWiFi() {
    LOG_INFO("Starting WiFi time sync...");

    if (!_connectivity) {
        LOG_ERROR("No connectivity service for WiFi sync");
        return false;
    }

    bool success = _connectivity->ensureTimeSync();

    if (success) {
        setSyncResult(SyncSource::WIFI, true);
        LOG_INFO("WiFi sync success");
    } else {
        setSyncResult(SyncSource::WIFI, false);
        LOG_WARN("WiFi sync failed");
    }

    return success;
}

// ---- BLE Management ----

void TimeSyncService::startBLE() {
    if (_bleActive) return;

    LOG_INFO("Starting BLE for time sync");

    NimBLEDevice::init(BLE_DEVICE_NAME);

    // Create server
    g_serverCallback = new TimeServerCallback();
    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(g_serverCallback);
    _bleServer = server;

    // Create service
    NimBLEService* service = server->createService(SERVICE_UUID);
    _bleService = service;

    // Create write characteristic (phone writes time here)
    g_writeCallback = new TimeWriteCallback(this);
    NimBLECharacteristic* charTime = service->createCharacteristic(
        TIME_WRITE_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    charTime->setCallbacks(g_writeCallback);
    _bleCharTime = charTime;

    // Create read characteristic (phone reads device time)
    NimBLECharacteristic* charRead = service->createCharacteristic(
        TIME_READ_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    // Set initial value (current epoch if RTC available)
    if (_rtc) {
        uint32_t epoch = _rtc->getEpochTime();
        charRead->setValue((uint8_t*)&epoch, 4);
    }

    // Start advertising
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    NimBLEDevice::startAdvertising();

    _bleActive = true;
    LOG_INFO("BLE advertising started: %s", BLE_DEVICE_NAME);
}

void TimeSyncService::stopBLE() {
    if (!_bleActive) return;

    LOG_INFO("Stopping BLE");
    NimBLEDevice::stopAdvertising();

    // Clean up callbacks BEFORE deinit — deinit(true) causes double-free on ESP32-C6
    if (g_writeCallback) {
        delete g_writeCallback;
        g_writeCallback = nullptr;
    }
    if (g_serverCallback) {
        delete g_serverCallback;
        g_serverCallback = nullptr;
    }

    // Use deinit(false) — deinit(true) causes heap corruption on Tasmota ESP32-C6.
    // Memory is reclaimed by deep sleep reset anyway.
    NimBLEDevice::deinit(false);

    _bleActive = false;
    _bleServer = nullptr;
    _bleService = nullptr;
    _bleCharTime = nullptr;
}

bool TimeSyncService::isBLEAdvertising() const {
    return _bleActive;
}

// ---- Mode Management ----

void TimeSyncService::setMode(SyncMode mode) {
    _mode = mode;
    _status.mode = mode;
    saveConfig();
    LOG_INFO("Sync mode changed to %s", modeToString(mode));
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

// ---- Status ----

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

// ---- Internal ----

void TimeSyncService::setSyncResult(SyncSource source, bool success) {
    _status.lastSource = source;
    if (success) {
        _status.lastSyncTime = _rtc ? _rtc->getEpochTime() : 0;
    }
}

// ---- Config Persistence ----

void TimeSyncService::saveConfig() {
    File file = LittleFS.open(TIME_SYNC_CONFIG_FILE, "w");
    if (!file) {
        LOG_ERROR("Failed to save sync config");
        return;
    }
    file.printf("mode=%d\n", static_cast<int>(_mode));
    file.close();
    LOG_INFO("Sync config saved: mode=%d", static_cast<int>(_mode));
}

void TimeSyncService::loadConfig() {
    File file = LittleFS.open(TIME_SYNC_CONFIG_FILE, "r");
    if (!file) {
        LOG_INFO("No sync config found, using default");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("mode=")) {
            int modeVal = line.substring(5).toInt();
            if (modeVal >= 0 && modeVal <= static_cast<int>(SyncMode::WIFI_FIRST)) {
                _mode = static_cast<SyncMode>(modeVal);
                _status.mode = _mode;
                LOG_INFO("Loaded sync mode: %s", modeToString(_mode));
            }
        }
    }
    file.close();
}

#endif // !MOCK
