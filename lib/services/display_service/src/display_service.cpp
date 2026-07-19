#include "display_service.h"
#include <logger.h>

DisplayService::DisplayService(DisplayManager* display, RTCManager* rtc, ConnectivityService* connectivity)
    : displayManager(display), rtcManager(rtc), connectivityService(connectivity),
      lastDisplayedReading(), displayNeedsUpdate(true) {
}

bool DisplayService::showStartupScreen() {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for startup screen");
        return false;
    }

    displayManager->clear();
    displayManager->showMessage("Booting...");

    // Brief pause so user can see the boot message
    delay(500);

    // Build status display
    SystemStatus status;
    status.sensorOk = true;   // Will be set by caller if sensor fails
    status.displayOk = displayManager->isReady();
    status.storageOk = true;  // Will be set by caller if storage fails
    status.rtcOk = true;
    status.wifiOk = (connectivityService && connectivityService->isConnected());

    displayManager->showSystemStatus(status);

    LOG_INFO("Startup screen displayed");
    return true;
}

bool DisplayService::showCurrentReading(const SensorReading& reading, const String& timeStr) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for reading display");
        return false;
    }

    if (!reading.isValid) {
        LOG_WARN("Cannot display invalid reading");
        return false;
    }

    displayManager->showReading(reading, timeStr);
    lastDisplayedReading = reading;
    displayNeedsUpdate = false;

    LOG_INFO("Sensor reading displayed: T=%.1f H=%.1f P=%.0f",
             reading.temperature, reading.humidity, reading.pressure);
    return true;
}

bool DisplayService::showHistoricalStats(const TemperatureStats& stats) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for stats display");
        return false;
    }

    displayManager->showTemperatureStats(stats);
    LOG_INFO("Temperature stats displayed");
    return true;
}

bool DisplayService::showSystemStatus(const String& status) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for system status");
        return false;
    }

    displayManager->showMessage(status.c_str());
    return true;
}

bool DisplayService::showErrorScreen(const String& error) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for error screen");
        return false;
    }

    displayManager->showError(error.c_str());
    LOG_ERROR("Error screen displayed: %s", error.c_str());
    return true;
}

void DisplayService::forceUpdate() {
    displayNeedsUpdate = true;
}

bool DisplayService::needsUpdate(const SensorReading& newReading) const {
    if (displayNeedsUpdate) return true;
    if (!newReading.isValid) return false;

    // Check if any value changed meaningfully
    if (newReading.temperature != lastDisplayedReading.temperature) return true;
    if (newReading.humidity != lastDisplayedReading.humidity) return true;
    if (newReading.pressure != lastDisplayedReading.pressure) return true;

    return false;
}

void DisplayService::clear() {
    if (displayManager && displayManager->isReady()) {
        displayManager->clear();
    }
}

void DisplayService::turnOff() {
    if (displayManager) {
        displayManager->disconnect();
        LOG_INFO("Display service: display turned off");
    }
}

void DisplayService::turnOn() {
    if (displayManager) {
        displayManager->reconnect();
        displayNeedsUpdate = true;
        LOG_INFO("Display service: display turned on");
    }
}
