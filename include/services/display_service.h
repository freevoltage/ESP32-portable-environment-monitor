#pragma once
#include "../hardware/display.h"
#include "data_structures.h"

class DisplayService{
    private:
        DisplayManager* displayManager;
        SensorReading lastDisplayedReading;
        bool displayNeedsUpdate = true;


    public: 
        DisplayService(DisplayManager* display);

        // Display Operations
        // TODO The Compilation fails when trying to compile with this function
        bool showCurrentReading(const SensorReading& reading, const String& timeStr);
        
        bool showHistoricalStats(const TemperatureStats& stats);
        bool showSystemStatus(const String& status);
        bool showStartupScreen();
        bool showErrorScreen(const String& error);

        // Display Management
        void forceUpdate();
        bool needsUpdate(const SensorReading& newReading) const;
        void clear();

        // Power Management
        void turnOff();
        void turnOn();
};
