#ifndef MOCK
#include "battery_manager.h"
#include <config.h>
#include <Wire.h>
#include <logger.h>

BatteryManager::BatteryManager() : _initialized(false) {}

bool BatteryManager::begin() {
    LOG_INFO("BatteryManager begin()");

    enableI2CPower();
    delay(200); // Let I2C rail and MAX17048 ADC stabilize after power-on

    _initialized = _fuelGauge.begin(&Wire);
    if (!_initialized) {
        LOG_ERROR("Could not find MAX17048 fuel gauge!");
        return false;
    }

    LOG_INFO("MAX17048 initialized, chip ID: 0x%02X", _fuelGauge.getChipID());

    // Wait for first valid ADC conversion
    delay(100);
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

    // Sanity check: a real LiPo always reads > 0V. 0V means no battery or gauge not ready.
    if (status.voltage < 1.0f) {
        LOG_WARN("Battery voltage too low (%.2fV) — no battery or gauge not ready", status.voltage);
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
