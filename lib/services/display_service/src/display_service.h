#pragma once
#ifdef MOCK
    #include <ctime>
    typedef std::string String;
#else
    #include <Arduino.h>
#endif
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

        // Display Management
        void forceUpdate();
        bool needsUpdate(const SensorReading& newReading) const;
        void clear();

        // Power Management
        void turnOff();
        void turnOn();
};
