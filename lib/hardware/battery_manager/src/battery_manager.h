#pragma once

#include <data_structures.h>

#ifndef MOCK
#include <Adafruit_MAX1704X.h>
#endif

class BatteryManager {
public:
    BatteryManager();

    bool begin();
    bool isReady();

    BatteryStatus getStatus();

    void enableI2CPower();
    void disableI2CPower();

private:
#ifndef MOCK
    Adafruit_MAX17048 _fuelGauge;
#endif
    bool _initialized;
};
