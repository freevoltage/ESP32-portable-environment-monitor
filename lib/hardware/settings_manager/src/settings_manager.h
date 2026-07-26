#pragma once
#ifdef MOCK
    #include <string>
    typedef std::string String;
#else
    #include <Arduino.h>
    #include <LittleFS.h>
#endif
#include "data_structures.h"

class SettingsManager {
public:
    SettingsManager();

    bool begin();
    const DeviceSettings& getSettings() const;
    void cycleMeasurementInterval();
    void cycleNTPSyncInterval();
    bool save();
    void apply();

private:
    DeviceSettings _settings;
    bool _initialized;

    bool load();
};
