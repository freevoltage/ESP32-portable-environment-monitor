/*
    Display Manager Class to support the
    Adafruit 1.54" 240x240 Color TFT IPS Breakout LCD Display
    Also supports 1.14" 240x135 (change init() resolution and rotation)
    See:
    https://learn.adafruit.com/adafruit-1-14-240x135-color-_tft-breakout
*/

#include "display_manager.h"
#include <logger.h>

// Backlight polarity: set TFT_BACKLIGHT_INVERTED in config.h
#define BL_OFF  (TFT_BACKLIGHT_INVERTED ? LOW : HIGH)
#define BL_ON   (TFT_BACKLIGHT_INVERTED ? HIGH : LOW)

DisplayManager::DisplayManager() : 
    _tft(nullptr),
    _cs(TFT_CS),
    _dc(TFT_DC),
    _rst(TFT_RST),
    _lit(TFT_LIT),
    _initialized(false),
    _isOn(false) {
    // Immediately deselect CS pin
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);

    //pinMode(_cs, OUTPUT);
    //pinMode(_dc, OUTPUT);
    //pinMode(_rst, OUTPUT);
    pinMode(_lit, OUTPUT);
    digitalWrite(_lit, BL_OFF);
}

DisplayManager::DisplayManager(uint8_t tft_cs, uint8_t tft_dc, uint8_t tft_rst, uint8_t tft_lit) : 
    _tft(nullptr),
    _cs(tft_cs),
    _dc(tft_dc),
    _rst(tft_rst),
    _lit(tft_lit),
    _initialized(false),
    _isOn(false)
{
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);

    pinMode(_lit, OUTPUT);
    digitalWrite(_lit, BL_OFF);
}

DisplayManager::~DisplayManager()
{
    if (_tft)
    {
        delete _tft;
        _tft = nullptr;
    }
}

bool DisplayManager::begin()
{
    if(_initialized){
        LOG_WARN("Display already initialized.");
        return true;
    }

    _tft = new Adafruit_ST7789(_cs, _dc, _rst);

    if(!_tft){
        LOG_ERROR("Failed to allocate display object");
        return false;
    }

    _tft->init(240, 240);
    _tft->setRotation(TFT_ROTATION);
    clear();

    digitalWrite(_lit, BL_ON);
    _initialized = true;
    _isOn = true;
    LOG_INFO("Display initialized");
    return true;
}

bool DisplayManager::isReady()
{
    // Check if the Display is initialized and that the _tft is not a Nullpointer.
    return (_initialized && _tft != nullptr);
}

void DisplayManager::disconnect()
{
    if (!isReady())
        return;

    LOG_INFO("Powering off display...");

    clear();

    // Send display to sleep mode
    _tft->sendCommand(ST77XX_DISPOFF); // Display OFF
    delay(50);
    _tft->enableSleep(true);
    delay(120);

    // Turn off Backlight
    digitalWrite(_lit, BL_OFF);

    delay(120);
    digitalWrite(TFT_LIT, BL_OFF);

    // Note: Don't call SPI.end() to keep the bus available for the SD card
    _initialized = false;
    _isOn = false;
    LOG_INFO("Display powered off");
}

void DisplayManager::reconnect()
{
    if (!_tft)
        return;

    LOG_INFO("Reconnecting display...");

    // Wake up display
    _tft->sendCommand(ST77XX_SLPOUT); // Sleep OUT
    delay(120);
    _tft->enableSleep(false); // Display ON

    // Turn on backlight
    digitalWrite(_lit, BL_ON);

    _initialized = true;
    _isOn = true;
    LOG_INFO("Display reconnected");
}

void DisplayManager::clear()
{
    if (!isReady())
        return;
    _tft->fillScreen(ST77XX_BLACK);
}

void DisplayManager::showReading(const SensorReading &reading, const String &dateTime)
{
    if (!isReady())
        return;

    clear();
    _tft->setCursor(5, 5);

    // Header
    _tft->setTextColor(ST77XX_CYAN);
    _tft->setTextSize(1);
    _tft->println("SENSOR READING");

    // DateTime
    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("Time: %s\n\n", dateTime.c_str());

    // Temperature
    _tft->setTextColor(ST77XX_RED);
    _tft->setTextSize(2);
    _tft->printf("%.1f°C\n", reading.temperature);

    // Humidity
    _tft->setTextColor(ST77XX_BLUE);
    _tft->setTextSize(1);
    _tft->printf("Humidity: %.1f%%\n", reading.humidity);

    // Pressure
    _tft->setTextColor(ST77XX_GREEN);
    _tft->printf("Pressure: %.0f hPa\n", reading.pressure);

    // Altitude (if available)
    if (reading.pressure != 0)
    {
        _tft->setTextColor(ST77XX_YELLOW);
        _tft->printf("Pressure: %.0f m\n", reading.pressure);
    }
}

void DisplayManager::showReading(const SensorReading &reading)
{
    if (!isReady())
        return;

    clear();
    _tft->setCursor(0, 16);

    // Configuration
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);

    // Timestamp
    _tft->printf("Time: %lu\n", reading.timestamp / 1000);

    // Temperature
    _tft->printf("%.1f %s", reading.temperature, "°C");

    // Humidity
    _tft->printf("Humidity: %.1f%%\n", reading.humidity);

    // Pressure
    _tft->printf("Pressure: %.0fhPa\n", reading.pressure);

    // Altitude
    // TODO Implement the displaying of the altitude
}

void DisplayManager::showTemperatureStats(const TemperatureStats &stats)
{
    if (!isReady())
        return;

    clear();
    drawHeader("TEMPERATURE STATS");

    _tft->setTextSize(1);
    _tft->setCursor(5, 20);

    // Min temperature
    _tft->setTextColor(ST77XX_CYAN);
    _tft->printf("MIN: %.1f°C\n", stats.min);

    // Max temperature
    _tft->setTextColor(ST77XX_RED);
    _tft->printf("MAX: %.1f°C\n", stats.max);

    // Average
    _tft->setTextColor(ST77XX_GREEN);
    _tft->printf("AVG: %.1f°C\n", stats.average);

    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("Samples: %d", stats.sampleCount);
}

void DisplayManager::showSystemStatus(const SystemStatus &status)
{
    if (!isReady())
        return;

    clear();
    drawHeader("SYSTEM STATUS");

    _tft->setTextSize(1);
    _tft->setCursor(5, 20);
    _tft->setTextColor(ST77XX_WHITE);

    _tft->printf("Sensor: ");
    _tft->setTextColor(status.sensorOk ? ST77XX_GREEN : ST77XX_RED);
    _tft->printf("%s\n", status.sensorOk ? "OK" : "FAIL");

    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("Storage: ");
    _tft->setTextColor(status.storageOk ? ST77XX_GREEN : ST77XX_RED);
    _tft->printf("%s\n", status.storageOk ? "OK" : "FAIL");

    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("RTC: ");
    _tft->setTextColor(status.rtcOk ? ST77XX_GREEN : ST77XX_RED);
    _tft->printf("%s\n", status.rtcOk ? "OK" : "FAIL");

    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("WiFi: ");
    _tft->setTextColor(status.wifiOk ? ST77XX_GREEN : ST77XX_RED);
    _tft->printf("%s\n", status.wifiOk ? "OK" : "FAIL");

    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("Memory: %lu KB", status.freeMemory / 1024);
}

void DisplayManager::showConnectivityStatus(const ConnectivityStatus &status)
{
    if (!isReady())
        return;

    clear();
    drawHeader("CONNECTIVITY");

    _tft->setTextSize(1);
    _tft->setCursor(5, 20);

    // WiFi status
    _tft->setTextColor(ST77XX_WHITE);
    _tft->print("WiFi: ");
    switch (status) {
        case ConnectivityStatus::DISCONNECTED:
            _tft->setTextColor(ST77XX_RED);
            _tft->println("Disconnected");
            break;
        case ConnectivityStatus::CONNECTING:
            _tft->setTextColor(ST77XX_YELLOW);
            _tft->println("Connecting...");
            break;
        case ConnectivityStatus::CONNECTED:
        case ConnectivityStatus::TIME_SYNCING:
        case ConnectivityStatus::SYNC_COMPLETE:
            _tft->setTextColor(ST77XX_GREEN);
            _tft->println("Connected");
            break;
    }

    // NTP status
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(5, 40);
    _tft->print("NTP:  ");
    switch (status) {
        case ConnectivityStatus::DISCONNECTED:
        case ConnectivityStatus::CONNECTING:
        case ConnectivityStatus::CONNECTED:
            _tft->setTextColor(ST77XX_YELLOW);
            _tft->println("Waiting");
            break;
        case ConnectivityStatus::TIME_SYNCING:
            _tft->setTextColor(ST77XX_YELLOW);
            _tft->println("Syncing...");
            break;
        case ConnectivityStatus::SYNC_COMPLETE:
            _tft->setTextColor(ST77XX_GREEN);
            _tft->println("Synced");
            break;
    }

    // State label
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(5, 65);
    _tft->print("State: ");
    switch (status) {
        case ConnectivityStatus::DISCONNECTED:
            _tft->setTextColor(ST77XX_RED);
            _tft->println("IDLE");
            break;
        case ConnectivityStatus::CONNECTING:
            _tft->setTextColor(ST77XX_YELLOW);
            _tft->println("CONNECTING");
            break;
        case ConnectivityStatus::CONNECTED:
            _tft->setTextColor(ST77XX_GREEN);
            _tft->println("CONNECTED");
            break;
        case ConnectivityStatus::TIME_SYNCING:
            _tft->setTextColor(ST77XX_YELLOW);
            _tft->println("TIME SYNC");
            break;
        case ConnectivityStatus::SYNC_COMPLETE:
            _tft->setTextColor(ST77XX_GREEN);
            _tft->println("READY");
            break;
    }
}

void DisplayManager::showMessage(const char *message)
{
    if (!_initialized)
        return;

    clear();
    _tft->setTextSize(2);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(5, 50);
    _tft->println(message);
}

void DisplayManager::showError(const char *message)
{
    if (!isReady())
        return;

    clear();
    drawHeader("ERROR");

    _tft->setTextSize(1);
    _tft->setTextColor(ST77XX_RED);
    _tft->setCursor(5, 25);

    // Word wrap for long error messages
    _tft->println(message);
}

bool DisplayManager::testConnection()
{
    if (!_tft)
        return false;

    // Simple test - try to fill screen
    _tft->fillScreen(ST77XX_RED);
    delay(100);
    _tft->fillScreen(ST77XX_GREEN);
    delay(100);
    _tft->fillScreen(ST77XX_BLUE);
    delay(100);
    clear();

    return true;
}

void DisplayManager::setBrightness(uint8_t brightness)
{
    // PWM duty cycle: 255 = full brightness (BL_ON level), 0 = off (BL_OFF level)
    // Polarity is already handled by BL_ON/BL_OFF macros in digitalWrite calls
    analogWrite(TFT_LIT, brightness);
}

void DisplayManager::drawHeader(const char *title)
{
    _tft->setTextColor(ST77XX_CYAN);
    _tft->setTextSize(1);
    _tft->setCursor(5, 5);
    _tft->println(title);

    // Draw underline
    drawSeparator(15);
}

// param y: height of the separator
void DisplayManager::drawSeparator(int y)
{
    _tft->drawFastHLine(0, y, _tft->width(), ST77XX_WHITE);
}

void DisplayManager::drawGraph(const char *title, const char *unit,
                               const std::vector<float> &values,
                               const std::vector<time_t> &timestamps,
                               float minVal, float maxVal)
{
    if (!isReady() || values.empty())
        return;

    clear();
    drawHeader(title);

    int16_t originX = GRAPH_PADDING;
    int16_t originY = GRAPH_PADDING + GRAPH_HEIGHT;
    int16_t graphW = GRAPH_WIDTH;
    int16_t graphH = GRAPH_HEIGHT;

    // Draw axes
    _tft->drawFastVLine(originX, originY - graphH, graphH, ST77XX_WHITE);
    _tft->drawFastHLine(originX, originY, graphW, ST77XX_WHITE);

    // Min/Max labels
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(1);
    _tft->setCursor(2, originY - graphH);
    _tft->printf("%.0f", maxVal);
    _tft->setCursor(2, originY - 8);
    _tft->printf("%.0f", minVal);

    // Unit label at top right
    _tft->setCursor(originX + graphW - 20, 5);
    _tft->print(unit);

    // Draw data points connected by lines
    uint16_t count = values.size();
    float range = maxVal - minVal;
    if (range <= 0) range = 1.0f;

    for (uint16_t i = 0; i < count; i++)
    {
        int16_t x = originX + (i * graphW) / (count > 1 ? (count - 1) : 1);
        float normalized = (values[i] - minVal) / range;
        if (normalized < 0) normalized = 0;
        if (normalized > 1) normalized = 1;
        int16_t y = originY - (int16_t)(normalized * graphH);

        // Draw point
        _tft->fillCircle(x, y, 2, ST77XX_GREEN);

        // Draw line to previous point
        if (i > 0)
        {
            int16_t prevX = originX + ((i - 1) * graphW) / (count > 1 ? (count - 1) : 1);
            float prevNorm = (values[i - 1] - minVal) / range;
            if (prevNorm < 0) prevNorm = 0;
            if (prevNorm > 1) prevNorm = 1;
            int16_t prevY = originY - (int16_t)(prevNorm * graphH);
            _tft->drawLine(prevX, prevY, x, y, ST77XX_GREEN);
        }
    }

    // Current value in cyan below graph
    float current = values.back();
    _tft->setTextColor(ST77XX_CYAN);
    _tft->setTextSize(2);
    _tft->setCursor(originX, originY + 5);
    _tft->printf("%.1f%s", current, unit);

    // Timestamp of latest reading
    if (timestamps.size() > 0)
    {
        time_t latest = timestamps.back();
        struct tm *tm_info = localtime(&latest);
        char timeBuf[6];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", tm_info);
        _tft->setTextColor(ST77XX_YELLOW);
        _tft->setTextSize(1);
        _tft->setCursor(originX, originY + 25);
        _tft->printf("Last: %s", timeBuf);
    }
}

void DisplayManager::showOTAMode(const char* ip)
{
    if (!isReady())
        return;

    clear();

    // Title
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setTextSize(2);
    _tft->setCursor(50, 30);
    _tft->print("OTA MODE");

    // Separator
    drawSeparator(60);

    // Instructions
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(1);
    _tft->setCursor(20, 80);
    _tft->print("Open in browser:");
    _tft->setCursor(20, 100);
    _tft->setTextColor(ST77XX_CYAN);
    _tft->printf("http://%s/update", ip);

    // Status
    _tft->setTextColor(ST77XX_GREEN);
    _tft->setTextSize(2);
    _tft->setCursor(30, 150);
    _tft->print("Waiting for");

    _tft->setCursor(55, 175);
    _tft->print("upload...");

    // Bottom info
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setTextSize(1);
    _tft->setCursor(20, 210);
    _tft->print("Auth: admin / hikingstation");
    _tft->setCursor(20, 225);
    _tft->print("B=Exit  Timeout=120s");
}

void DisplayManager::showOTAProgress(int percent, size_t current, size_t total)
{
    if (!isReady())
        return;

    // Clear progress area
    _tft->fillRect(0, 100, 240, 120, ST77XX_BLACK);

    // Percentage
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(3);
    _tft->setCursor(80, 110);
    _tft->printf("%d%%", percent);

    // Progress bar
    int barX = 20, barY = 155, barW = 200, barH = 25;
    _tft->drawRect(barX, barY, barW, barH, ST77XX_WHITE);
    int fillW = (barW * percent) / 100;
    if (fillW > 0) {
        _tft->fillRect(barX + 1, barY + 1, fillW - 2, barH - 2, ST77XX_GREEN);
    }

    // Bytes info
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setTextSize(1);
    _tft->setCursor(50, 190);
    _tft->printf("%u / %u KB", (unsigned int)(current / 1024), (unsigned int)(total / 1024));
}

void DisplayManager::showBatteryInfo(const BatteryStatus& battery)
{
    if (!isReady()) return;

    // Small battery info block at bottom of screen
    int y = 220;
    _tft->setTextSize(1);

    if (!battery.isValid) {
        _tft->setTextColor(ST77XX_RED);
        _tft->setCursor(5, y);
        _tft->print("BATT: --");
        return;
    }

    // Color based on percentage
    if (battery.percent > 50) {
        _tft->setTextColor(ST77XX_GREEN);
    } else if (battery.percent > 20) {
        _tft->setTextColor(ST77XX_YELLOW);
    } else {
        _tft->setTextColor(ST77XX_RED);
    }

    _tft->setCursor(5, y);
    _tft->printf("BATT: %.0f%% (%.2fV)", battery.percent, battery.voltage);
}

void DisplayManager::showDashboard(const SensorReading& reading, const char* timeStr,
                                   int selectedItem, const BatteryStatus& battery)
{
    if (!isReady()) return;

    clear();
    drawHeader("HIKING STATION");

    // ── Sensor data (large, top third) ──────────────────────────────
    // Temperature
    _tft->setTextSize(3);
    _tft->setTextColor(ST77XX_RED);
    _tft->setCursor(5, 30);
    _tft->printf("%.1fC", reading.temperature);

    // Humidity (same line, right side)
    _tft->setTextColor(ST77XX_CYAN);
    _tft->setCursor(130, 30);
    _tft->printf("%.0f%%", reading.humidity);

    // Altitude (second line)
    _tft->setTextSize(2);
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setCursor(5, 60);
    _tft->printf("%.0f m", reading.altitude);

    // Pressure (same line, right side)
    _tft->setTextColor(ST77XX_GREEN);
    _tft->setCursor(120, 60);
    _tft->printf("%.0fhPa", reading.pressure);

    // ── Time (middle) ───────────────────────────────────────────────
    _tft->setTextSize(2);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(5, 90);
    _tft->print(timeStr);

    // ── Separator ───────────────────────────────────────────────────
    drawSeparator(115);

    // ── Menu items ──────────────────────────────────────────────────
    const char* items[] = {"Log Comfort", "Menu"};
    const int itemCount = 2;

    for (int i = 0; i < itemCount; i++) {
        if (i == selectedItem)
            _tft->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        else
            _tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);

        _tft->setTextSize(2);
        _tft->setCursor(10, 125 + i * 25);
        if (i == selectedItem) _tft->print("> ");
        else _tft->print("  ");
        _tft->print(items[i]);
    }

    // ── Battery bar ─────────────────────────────────────────────────
    if (battery.isValid) {
        int barY = 190;
        int barWidth = 80;
        int barHeight = 10;

        // Battery outline
        _tft->drawRect(5, barY, barWidth, barHeight, ST77XX_WHITE);

        // Fill based on percentage
        int fillWidth = (int)(barWidth * battery.percent / 100.0f);
        uint16_t color = battery.percent > 50 ? ST77XX_GREEN :
                         battery.percent > 20 ? ST77XX_YELLOW : ST77XX_RED;
        if (fillWidth > 0)
            _tft->fillRect(6, barY + 1, fillWidth - 1, barHeight - 2, color);

        // Percentage text
        _tft->setTextSize(1);
        _tft->setTextColor(color);
        _tft->setCursor(90, barY);
        _tft->printf("%.0f%% %.2fV", battery.percent, battery.voltage);
    }

    // ── Footer ──────────────────────────────────────────────────────
    _tft->setTextSize(1);
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setCursor(5, 225);
    _tft->print("A=Navigate B=Select");
}

void DisplayManager::showSyncUI(SyncMode currentMode, SyncSource lastSource, time_t lastSyncTime)
{
    if (!isReady()) return;

    clear();
    drawHeader("TIME SYNC");

    // Current mode
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);
    _tft->setCursor(10, 35);
    _tft->print("Mode:");

    // Mode value (highlighted in cyan)
    _tft->setTextColor(ST77XX_CYAN);
    _tft->setCursor(10, 60);
    const char* modeNames[] = {"OFF", "BLE", "WiFi", "BLE+WiFi", "WiFi+BLE"};
    _tft->print(modeNames[static_cast<int>(currentMode)]);

    // Last sync info
    _tft->setTextSize(1);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(10, 100);
    _tft->print("Last sync:");

    if (lastSource == SyncSource::NONE || lastSyncTime == 0) {
        _tft->setTextColor(ST77XX_YELLOW);
        _tft->setCursor(10, 115);
        _tft->print("Never");
    } else {
        _tft->setTextColor(lastSource == SyncSource::BLE ? ST77XX_GREEN : ST77XX_BLUE);
        _tft->setCursor(10, 115);
        const char* srcName = (lastSource == SyncSource::BLE) ? "BLE" : "WiFi";
        _tft->printf("%s @ %lu", srcName, (unsigned long)lastSyncTime);
    }

    // Button hints
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setTextSize(1);
    _tft->setCursor(5, 225);
    _tft->print("A=Mode B=Sync");
}

void DisplayManager::showSyncSubMenu(int selectedItem, SyncMode currentMode, SyncSource lastSource, time_t lastSyncTime)
{
    if (!isReady()) return;

    clear();
    drawHeader("TIME SYNC");

    const char* items[] = {"Mode", "Sync Now", "Back"};
    const char* modeNames[] = {"OFF", "BLE", "WiFi", "BLE+WiFi", "WiFi+BLE"};
    const int itemCount = 3;

    for (int i = 0; i < itemCount; i++)
    {
        if (i == selectedItem)
            _tft->setTextColor(ST77XX_BLACK, ST77XX_CYAN);
        else
            _tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);

        _tft->setTextSize(2);
        _tft->setCursor(10, 35 + i * 30);
        _tft->print(items[i]);

        // Show current value next to "Mode"
        if (i == 0) {
            _tft->setTextColor(ST77XX_CYAN);
            _tft->setCursor(100, 35);
            _tft->print(modeNames[static_cast<int>(currentMode)]);
        }
    }

    // Last sync info
    _tft->setTextSize(1);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(10, 140);
    _tft->print("Last sync:");

    if (lastSource == SyncSource::NONE || lastSyncTime == 0) {
        _tft->setTextColor(ST77XX_YELLOW);
        _tft->setCursor(10, 155);
        _tft->print("Never");
    } else {
        _tft->setTextColor(lastSource == SyncSource::BLE ? ST77XX_GREEN : ST77XX_BLUE);
        _tft->setCursor(10, 155);
        const char* srcName = (lastSource == SyncSource::BLE) ? "BLE" : "WiFi";
        _tft->printf("%s @ %lu", srcName, (unsigned long)lastSyncTime);
    }

    // Button hints
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setTextSize(1);
    _tft->setCursor(5, 225);
    _tft->print("A=Navigate B=Select");
}

void DisplayManager::showSyncProgress(const char* message)
{
    if (!isReady()) return;

    // Clear bottom area for progress
    _tft->fillRect(0, 140, 240, 80, ST77XX_BLACK);

    _tft->setTextColor(ST77XX_YELLOW);
    _tft->setTextSize(2);
    _tft->setCursor(20, 150);
    _tft->print(message);

    // Animated dots
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(1);
    _tft->setCursor(20, 175);
    _tft->print("Please wait...");
}
