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

bool DisplayService::showClock(const String& timeStr, const String& dateStr) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for clock");
        return false;
    }

    displayManager->clear();
    displayManager->drawHeader("CLOCK");

    Adafruit_ST7789* tft = displayManager->getTFT();

    tft->setTextSize(2);
    tft->setTextColor(ST77XX_GREEN);
    tft->setCursor(20, 60);
    tft->print(timeStr);

    tft->setTextSize(1);
    tft->setTextColor(ST77XX_WHITE);
    tft->setCursor(20, 100);
    tft->print(dateStr);

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

bool DisplayService::showMenu(DisplayMenu current) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for menu");
        return false;
    }

    displayManager->clear();
    displayManager->drawHeader("MENU");

    const char* items[] = {"Graph Temp", "Graph Humidity", "Graph Altitude", "Log Comfort", "OTA", "Sync Time", "Sleep"};
    const int itemCount = 7;

    for (int i = 0; i < itemCount; i++)
    {
        if (i == static_cast<int>(current))
        {
            displayManager->getTFT()->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        }
        else
        {
            displayManager->getTFT()->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        }
        displayManager->getTFT()->setTextSize(1);
        displayManager->getTFT()->setCursor(10, 22 + i * 28);
        displayManager->getTFT()->print(items[i]);
    }

    // Button hints
    displayManager->getTFT()->setTextColor(ST77XX_YELLOW);
    displayManager->getTFT()->setTextSize(1);
    displayManager->getTFT()->setCursor(5, 225);
    displayManager->getTFT()->print("A=Navigate B=Select");

    LOG_INFO("Menu displayed, selection=%d", static_cast<int>(current));
    return true;
}

bool DisplayService::showComfortUI(ComfortLevel current) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for comfort UI");
        return false;
    }

    displayManager->clear();
    displayManager->drawHeader("HOW DO YOU FEEL?");

    const char* labels[] = {"Too cold", "Cold", "Comfortable", "Warm", "Too warm"};
    const int itemCount = 5;

    for (int i = 0; i < itemCount; i++)
    {
        if (i == static_cast<int>(current))
        {
            displayManager->getTFT()->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        }
        else
        {
            displayManager->getTFT()->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        }
        displayManager->getTFT()->setTextSize(2);
        displayManager->getTFT()->setCursor(20, 30 + i * 30);
        displayManager->getTFT()->print(labels[i]);
    }

    displayManager->getTFT()->setTextColor(ST77XX_YELLOW);
    displayManager->getTFT()->setTextSize(1);
    displayManager->getTFT()->setCursor(5, 225);
    displayManager->getTFT()->print("A=Navigate B=Select");

    LOG_INFO("Comfort UI displayed, selection=%d", static_cast<int>(current));
    return true;
}

bool DisplayService::showGraph(const char* title, const char* unit,
                               const std::vector<float>& values,
                               const std::vector<time_t>& timestamps,
                               float minVal, float maxVal) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for graph");
        return false;
    }

    displayManager->drawGraph(title, unit, values, timestamps, minVal, maxVal);
    LOG_INFO("Graph displayed: %s", title);
    return true;
}

bool DisplayService::showSyncUI(SyncMode currentMode, SyncSource lastSource, time_t lastSyncTime) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for sync UI");
        return false;
    }

    displayManager->showSyncUI(currentMode, lastSource, lastSyncTime);
    LOG_INFO("Sync UI displayed, mode=%d", static_cast<int>(currentMode));
    return true;
}
