#pragma once
#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include <data_structures.h>
#include <config.h>

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Initialize display
    void begin(uint8_t tft_cs, uint8_t tft_dc, uint8_t tft_rst, uint8_t tft_lit);
    bool isReady();

    // Display Control
    void disconnect();
    void reconnect();  // Not sure if I need this actually
    void clear();
    
    // Content Display Methods
    // I am not sure which of those function I should actually use.
    void showReading(const SensorReading& reading, const String& dateTime);
    void showReading(const SensorReading& reading);

    void showTemperatureStats(const TemperatureStats& stats);
    void showSystemStatus(const SystemStatus& status);
    void showConnectivityStatus(const ConnectivityStatus& status);

    void showMessage(const char* message);
    void showError(const char* message);

    // Utility Methods
    // TODO Remove the testConnection method
    bool testConnection();
    void setBrightness(uint8_t brightness);

private:
    Adafruit_ST7789* _tft;

    uint8_t _cs, _dc, _rst, _lit;
    bool _initialized;
    bool _isOn;

    void drawHeader(const char* title);
    void drawSeparator(int y);
    // Internal helper
    
};
