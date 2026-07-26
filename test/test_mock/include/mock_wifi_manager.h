#pragma once
#include "wifi_manager.h"
#include <cstdio>

static bool mockWifiConnected = false;
static bool mockWifiShouldFail = false;
static bool mockNtpShouldFail = false;

WiFiManager::WiFiManager() : wifiConnected(false) {}

bool WiFiManager::connect(const char* ssid, const char* password, int timeoutSeconds, AbortCallback abort) {
    if (mockWifiShouldFail) {
        wifiConnected = false;
        return false;
    }
    wifiConnected = true;
    mockWifiConnected = true;
    return true;
}

void WiFiManager::disconnect() {
    wifiConnected = false;
    mockWifiConnected = false;
}

bool WiFiManager::isConnected() {
    return wifiConnected && mockWifiConnected;
}

String WiFiManager::getIPAddress() {
    if (isConnected()) return "192.168.1.100";
    return "Not connected";
}

int WiFiManager::getRSSI() {
    if (isConnected()) return -50;
    return 0;
}

bool WiFiManager::syncTimeNTP(const char* ntpServer, long gmtOffset, int daylightOffset) {
    if (!isConnected()) return false;
    if (mockNtpShouldFail) return false;
    return true;
}

extern "C" {
    void setMockWifiShouldFail(bool shouldFail) {
        mockWifiShouldFail = shouldFail;
    }

    void setMockNtpShouldFail(bool shouldFail) {
        mockNtpShouldFail = shouldFail;
    }

    void resetMockWifi() {
        mockWifiConnected = false;
        mockWifiShouldFail = false;
        mockNtpShouldFail = false;
    }
}
