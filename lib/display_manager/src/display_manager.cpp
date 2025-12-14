/*
    Display Manager Class to support the
    Adafruit 1.14" 240x135 Color _tft Breakout LCD Display
    See:
    https://learn.adafruit.com/adafruit-1-14-240x135-color-_tft-breakout
*/

#include "display_manager.h"
#include <logger.h>

DisplayManager::DisplayManager() : _tft(nullptr), _initialized(false)
{
    // Empty Constructor
}

DisplayManager::~DisplayManager()
{
    if (_tft)
    {
        delete _tft;
        _tft = nullptr;
    }
}

void DisplayManager::begin(uint8_t tft_cs, uint8_t tft_dc, uint8_t tft_rst, uint8_t tft_lit)
{
    _cs = tft_cs;
    _dc = tft_dc;
    _rst = tft_rst;
    _lit = tft_lit;

    // Actually cs, dc and rst should be defined as output in the display's library.
    pinMode(_cs, OUTPUT);
    pinMode(_dc, OUTPUT);
    pinMode(_rst, OUTPUT);
    pinMode(_lit, OUTPUT);

    // Ensure display is deselected initially
    digitalWrite(_cs, HIGH);
    digitalWrite(_lit, LOW);

    _tft = new Adafruit_ST7789(_cs, _dc, _rst);

    _tft->init(135, 240);
    _tft->setRotation(3);
    clear();

    digitalWrite(_lit, HIGH);
    _initialized = true;
    _isOn = true;
    LOG_INFO("Display initialized");
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
    digitalWrite(_lit, LOW);

    delay(120);
    digitalWrite(TFT_LIT, LOW);

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
    digitalWrite(_lit, HIGH);

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
    _tft->setTextColor(ST77XX_WHITE);

    // Add your connectivity status display logic here
    _tft->println("Status: Connected");
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
    // Use PWM to control backlight brightness (0-255)
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
