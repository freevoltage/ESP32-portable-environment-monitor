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

bool DisplayService::showDashboard(const SensorReading& reading, const String& timeStr,
                                   int selectedItem, const BatteryStatus& battery,
                                   bool wifiConnected, SyncSource lastSyncSource) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for dashboard");
        return false;
    }

    displayManager->showDashboard(reading, timeStr.c_str(), selectedItem, battery,
                                   wifiConnected, lastSyncSource);
    return true;
}

bool DisplayService::showMenu(DisplayMenu current) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for menu");
        return false;
    }

    displayManager->clear();
    displayManager->drawHeader("MENU");

    const char* items[] = {"Graph Temp", "Graph Humidity", "Graph Altitude", "Calendar", "Settings", "OTA", "Sync Time", "Back"};
    const int itemCount = 8;

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

bool DisplayService::showSyncSubMenu(int selectedItem, SyncMode currentMode, SyncSource lastSource, time_t lastSyncTime) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for sync sub-menu");
        return false;
    }

    displayManager->showSyncSubMenu(selectedItem, currentMode, lastSource, lastSyncTime);
    return true;
}

bool DisplayService::showSettingsSubMenu(int selectedItem, const DeviceSettings& settings) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for settings sub-menu");
        return false;
    }

    displayManager->showSettingsSubMenu(selectedItem, settings);
    return true;
}

bool DisplayService::showCalendarList(const std::vector<ComfortLog>& logs, int selectedIndex) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for calendar list");
        return false;
    }

    displayManager->clear();
    displayManager->drawHeader("COMFORT LOG");

    Adafruit_ST7789* tft = displayManager->getTFT();

    const char* levelNames[] = {"Too cold", "Cold", "Comfort.", "Warm", "Too warm"};
    uint16_t levelColors[] = {ST77XX_BLUE, ST77XX_CYAN, ST77XX_GREEN, ST77XX_YELLOW, ST77XX_RED};

    // Show up to 7 entries starting from the top
    int visibleCount = min((int)logs.size(), 7);
    for (int i = 0; i < visibleCount; i++) {
        int idx = i;  // logs should already be in reverse chronological order
        const ComfortLog& log = logs[idx];

        // Format date as DD-MM-YY
        struct tm* ti = localtime(&log.timestamp);
        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%02d-%02d-%02d",
                 ti->tm_mday, ti->tm_mon + 1, (ti->tm_year + 1900) % 100);

        // Highlight selected item
        if (idx == selectedIndex) {
            tft->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        } else {
            tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        }

        tft->setTextSize(1);
        tft->setCursor(10, 22 + i * 28);
        tft->print(dateBuf);

        // Show comfort level with color
        tft->setCursor(100, 22 + i * 28);
        if (idx == selectedIndex) {
            tft->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        } else {
            tft->setTextColor(levelColors[static_cast<int>(log.level)], ST77XX_BLACK);
        }
        tft->print(levelNames[static_cast<int>(log.level)]);
    }

    // Button hints
    tft->setTextColor(ST77XX_YELLOW);
    tft->setTextSize(1);
    tft->setCursor(5, 225);
    tft->print("A=Scroll B=Select");

    LOG_INFO("Calendar list displayed, %d entries, selected=%d", visibleCount, selectedIndex);
    return true;
}

bool DisplayService::showCalendarDetail(const char* dateStr, ComfortLevel level, bool hasLog, int selectedItem) {
    if (!displayManager || !displayManager->isReady()) {
        LOG_ERROR("Display not ready for calendar detail");
        return false;
    }

    displayManager->clear();
    displayManager->drawHeader(dateStr);

    Adafruit_ST7789* tft = displayManager->getTFT();

    if (hasLog) {
        // Show current comfort level
        const char* levelNames[] = {"Too cold", "Cold", "Comfortable", "Warm", "Too warm"};
        uint16_t levelColors[] = {ST77XX_BLUE, ST77XX_CYAN, ST77XX_GREEN, ST77XX_YELLOW, ST77XX_RED};

        tft->setTextSize(2);
        tft->setTextColor(levelColors[static_cast<int>(level)]);
        tft->setCursor(20, 50);
        tft->print(levelNames[static_cast<int>(level)]);

        // Menu options
        const char* options[] = {"Change", "Back"};
        for (int i = 0; i < 2; i++) {
            if (i == selectedItem) {
                tft->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
            } else {
                tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            }
            tft->setTextSize(2);
            tft->setCursor(20, 100 + i * 40);
            tft->printf("%s %s", i == selectedItem ? ">" : " ", options[i]);
        }
    } else {
        // No log for this day
        tft->setTextSize(2);
        tft->setTextColor(ST77XX_WHITE);
        tft->setCursor(20, 50);
        tft->print("No entry");

        // Menu options
        const char* options[] = {"Log it", "Back"};
        for (int i = 0; i < 2; i++) {
            if (i == selectedItem) {
                tft->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
            } else {
                tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            }
            tft->setTextSize(2);
            tft->setCursor(20, 100 + i * 40);
            tft->printf("%s %s", i == selectedItem ? ">" : " ", options[i]);
        }
    }

    // Button hints
    tft->setTextColor(ST77XX_YELLOW);
    tft->setTextSize(1);
    tft->setCursor(5, 225);
    tft->print("A=Navigate B=Select");

    LOG_INFO("Calendar detail displayed, hasLog=%d, selected=%d", hasLog, selectedItem);
    return true;
}
