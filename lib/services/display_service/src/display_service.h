#pragma once
#ifdef MOCK
    #include <ctime>
    typedef std::string String;
#else
    #include <Arduino.h>
#endif
#include <vector>
#include "display_manager.h"
#include "rtc_manager.h"
#include "connectivity_service.h"
#include "data_structures.h"

class DisplayService{
    private:
        DisplayManager* displayManager;
        RTCManager* rtcManager;
        ConnectivityService* connectivityService;
        SensorReading lastDisplayedReading;
        bool displayNeedsUpdate = true;

    public:
        DisplayService(DisplayManager* display, RTCManager* rtc, ConnectivityService* connectivity);

        // Display Operations
        bool showStartupScreen();
        bool showCurrentReading(const SensorReading& reading, const String& timeStr);
        bool showHistoricalStats(const TemperatureStats& stats);
        bool showSystemStatus(const String& status);
        bool showErrorScreen(const String& error);
        bool showClock(const String& timeStr, const String& dateStr);

        // Hiking Station UI
        bool showMenu(DisplayMenu current);
        bool showComfortUI(ComfortLevel current);
        bool showGraph(const char* title, const char* unit,
                       const std::vector<float>& values,
                       const std::vector<time_t>& timestamps,
                       float minVal, float maxVal);
        bool showSyncUI(SyncMode currentMode, SyncSource lastSource, time_t lastSyncTime);
        bool showSyncSubMenu(int selectedItem, SyncMode currentMode, SyncSource lastSource, time_t lastSyncTime);

        // Display Management
        void forceUpdate();
        bool needsUpdate(const SensorReading& newReading) const;
        void clear();

        // Power Management
        void turnOff();
        void turnOn();
};
