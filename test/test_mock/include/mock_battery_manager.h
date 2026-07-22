#pragma once
#include "battery_manager.h"
#include <cstdio>

static float mockBatteryVoltage = 4.15f;
static float mockBatteryPercent = 85.0f;
static float mockBatteryChargeRate = 0.5f;
static bool mockBatteryShouldFail = false;
static bool mockBatteryInitialized = false;
static bool mockI2CPowerEnabled = false;

BatteryManager::BatteryManager() : _initialized(false) {}

bool BatteryManager::begin() {
    if (mockBatteryShouldFail) {
        _initialized = false;
        return false;
    }
    _initialized = true;
    mockBatteryInitialized = true;
    return true;
}

bool BatteryManager::isReady() {
    return _initialized && !mockBatteryShouldFail;
}

BatteryStatus BatteryManager::getStatus() {
    BatteryStatus status;
    if (!_initialized || mockBatteryShouldFail) {
        return status;
    }
    status.voltage = mockBatteryVoltage;
    status.percent = mockBatteryPercent;
    status.chargeRate = mockBatteryChargeRate;
    status.isValid = true;
    return status;
}

void BatteryManager::enableI2CPower() {
    mockI2CPowerEnabled = true;
}

void BatteryManager::disableI2CPower() {
    mockI2CPowerEnabled = false;
}

extern "C" {
    void setMockBatteryVoltage(float voltage) {
        mockBatteryVoltage = voltage;
    }

    void setMockBatteryPercent(float percent) {
        mockBatteryPercent = percent;
    }

    void setMockBatteryShouldFail(bool shouldFail) {
        mockBatteryShouldFail = shouldFail;
    }

    void resetMockBattery() {
        mockBatteryVoltage = 4.15f;
        mockBatteryPercent = 85.0f;
        mockBatteryChargeRate = 0.5f;
        mockBatteryShouldFail = false;
        mockBatteryInitialized = false;
        mockI2CPowerEnabled = false;
    }
}
