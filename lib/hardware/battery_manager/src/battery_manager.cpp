#ifndef MOCK
#include "battery_manager.h"
#include <config.h>
#include <Wire.h>
#include <logger.h>

BatteryManager::BatteryManager() : _initialized(false) {}

bool BatteryManager::begin() {
    LOG_INFO("BatteryManager begin()");

    enableI2CPower();
    delay(50); // Let I2C rail stabilize

    _initialized = _fuelGauge.begin(&Wire);
    if (!_initialized) {
        LOG_ERROR("Could not find MAX17048 fuel gauge!");
        return false;
    }

    LOG_INFO("MAX17048 initialized, chip ID: 0x%02X", _fuelGauge.getChipID());
    return true;
}

bool BatteryManager::isReady() {
    return _initialized;
}

BatteryStatus BatteryManager::getStatus() {
    BatteryStatus status;

    if (!_initialized) {
        LOG_ERROR("Battery manager not initialized");
        return status;
    }

    status.voltage = _fuelGauge.cellVoltage();
    status.percent = _fuelGauge.cellPercent();
    status.chargeRate = _fuelGauge.chargeRate();

    // Validate readings
    if (isnan(status.voltage) || isnan(status.percent)) {
        LOG_ERROR("Battery gauge read failed");
        return status;
    }

    status.isValid = true;
    return status;
}

void BatteryManager::enableI2CPower() {
    pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
}

void BatteryManager::disableI2CPower() {
    digitalWrite(NEOPIXEL_I2C_POWER, LOW);
}

#endif // !MOCK
