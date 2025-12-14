#pragma once
#include "../hardware/wifi_manager.h"
#include "../hardware/rtc_manager.h"
#include "data_structures.h"

class ConnectivityService{
    private:
        WiFiManager* wiFiManager;
        RTCManager* rtcManager;

        ConnectivityStatus status = ConnectivityStatus::DISCONNECTED;
        unsigned long lastSyncAttempt = 0;
        const unsigned long SYNC_INTERVAL = 24 * 60 * 60 * 1000; // 24 hours

    public:

        ConnectivityService(WiFiManager* wifi, RTCManager* rtc);

        // Connectivity Manager
        bool ensureTimeSync();
        bool syncTimeIfNeeded();
        ConnectivityStatus getStatus() const;

        // Time Management
        bool isTimeSyncRequired() const;
        unsigned long getTimeSyncLastSync() const;

        // Connection Management
        bool connect(int timeoutMs = 10000);
        void disconnect();
        bool isConnected() const;
};