#pragma once

#ifdef MOCK
    #include <cstdint>
    #include <ctime>
    #include <functional>
    typedef std::string String;
#else
    #include <Arduino.h>
#endif

#include <data_structures.h>

// Forward declarations
#ifndef MOCK
class RTCManager;
class ConnectivityService;
#endif

class TimeSyncService {
public:
    // Progress callback: called with status messages during sync (e.g. "BLE 7s left")
    typedef void (*ProgressCallback)(const char* message);

    TimeSyncService();
    ~TimeSyncService();

    bool begin(RTCManager* rtc, ConnectivityService* connectivity);
    void stop();

    // Sync operations
    bool sync(ProgressCallback progress = nullptr);
    bool syncBLE(ProgressCallback progress = nullptr);
    bool syncWiFi();

    // Mode management
    void setMode(SyncMode mode);
    SyncMode getMode() const;
    static const char* modeToString(SyncMode mode);

    // Status
    SyncStatus getStatus() const;
    SyncSource getLastSyncSource() const;
    time_t getLastSyncTime() const;
    bool isSyncing() const;

    // BLE control
    void startBLE();
    void stopBLE();
    bool isBLEAdvertising() const;

    // Config persistence
    void saveConfig();
    void loadConfig();

    // BLE callback state (public for BLECharacteristicCallbacks access)
    volatile bool _bleTimeReceived;
    volatile uint32_t _pendingBLETime;

private:
#ifndef MOCK
    RTCManager* _rtc;
    ConnectivityService* _connectivity;

    // BLE objects (owned by this class when active)
    void* _bleServer;      // BLEServer*
    void* _bleService;     // BLEService*
    void* _bleCharTime;    // BLECharacteristic*
#endif

    SyncMode _mode;
    SyncStatus _status;
    bool _bleActive;
    bool _initialized;

    void setSyncResult(SyncSource source, bool success);
};
