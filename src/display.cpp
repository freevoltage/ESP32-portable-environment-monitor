/* 
    Display Manager Class to support the 
    Adafruit 1.14" 240x135 Color _tft Breakout LCD Display
    See:
    https://learn.adafruit.com/adafruit-1-14-240x135-color-_tft-breakout
*/

#include "hardware/display.h"

DisplayManager::DisplayManager() : _tft(nullptr), _initialized(false) {
    // Empty Constructor
}

DisplayManager::~DisplayManager(){
    if (_tft) {
        delete _tft;
        _tft = nullptr;
    }
}

void DisplayManager::begin(uint8_t tft_cs, uint8_t tft_dc, uint8_t tft_rst, uint8_t tft_lit) {
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
    _tft-> setRotation(3);
    clear();

    digitalWrite(_lit, HIGH);
    _initialized = true;
    Serial.println("Display initialized");
}

bool DisplayManager::isReady(){
    // Check if the Display is initialized and that the _tft is not a Nullpointer.
    return (_initialized && _tft != nullptr);
}

void DisplayManager::disconnect() {
    if (!isReady()) return;

    Serial.println("Powering off display...");
    
    clear();

    // Send display to sleep mode
    _tft->sendCommand(ST77XX_DISPOFF);    // Display OFF
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
    Serial.println("Display powered off");
}

void DisplayManager::reconnect(){
    if (!_tft) return;
    
    Serial.println("Reconnecting display...");
    
    // Wake up display
    _tft->sendCommand(ST77XX_SLPOUT);     // Sleep OUT
    delay(120);
    _tft->enableSleep(false);    // Display ON
    
    // Turn on backlight
    digitalWrite(_lit, HIGH);
    
    _initialized = true;
    _isOn = true;
    Serial.println("Display reconnected");
}


void DisplayManager::clear() {
    if(!isReady()) return;
    _tft->fillScreen(ST77XX_BLACK);
}

void DisplayManager::showReading(const SensorReading &reading)
{
    if (!isReady()) return;
    
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


void DisplayManager::showTemperatureStats(const TemperatureStats& stats) {
    // TODO Implement this function properly
    if (!_initialized) return;
    
    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(1);
    _tft->setCursor(5, 5);
    
    //_tft->printf("Last %dh Stats:\n\n", hours);
    
    // Min temperature
    _tft->setTextColor(ST77XX_CYAN);
    _tft->printf("MIN: %.1fC\n", stats.min);
    _tft->setTextColor(ST77XX_WHITE);
    //_tft->printf("at %s\n\n", stats.minTempTime.c_str());
    
    // Max temperature  
    _tft->setTextColor(ST77XX_RED);
    _tft->printf("MAX: %.1fC\n", stats.max);
    _tft->setTextColor(ST77XX_WHITE);
    //_tft->printf("at %s\n\n", stats.maxTempTime.c_str());
    
    // Average
    _tft->setTextColor(ST77XX_GREEN);
    _tft->printf("AVG: %.1fC\n\n", stats.average);
    
    _tft->setTextColor(ST77XX_WHITE);
    _tft->printf("Readings: %d", stats.sampleCount);
}


void DisplayManager::showSystemStatus(const SystemStatus & status){
    if (!_initialized) return;

    clear();
    drawHeader("SYSTEM STATUS");

    _tft->setTextSize(1);
    _tft->setCursor(0, 16);

    _tft->printf("Sensor: %s\n", status.sensorOk ? "OK" : "FAIL");
    _tft->printf("Storage: %s\n", status.storageOk ? "OK" : "FAIL");
    _tft->printf("RTC: %s\n", status.rtcOk ? "OK" : "FAIL");
    _tft->printf("WiFi: %s\n", status.wifiOk ? "OK" : "FAIL");
    _tft->printf("Memory: %luKB\n", status.freeMemory / 1024);
}

void DisplayManager::showConnectivityStatus(const ConnectivityStatus &status){
    if (!_initialized) return;
}

void DisplayManager::showMessage(const char *message){
    if (!_initialized) return;
    
    clear();
    _tft->setTextSize(2);
    _tft->setCursor(0, 0);
    _tft->println(message);
}

void DisplayManager::showError(const char *message){
    if (!_initialized) return;

    clear();
    drawHeader("ERROR");
    _tft->setTextSize(2);
    _tft->setCursor(0, 16);
    _tft->println(message);
}

