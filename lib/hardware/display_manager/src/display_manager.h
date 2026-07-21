#pragma once
#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include <data_structures.h>
#include <config.h>
#include <vector>


class DisplayManager {
public:
    DisplayManager();
    DisplayManager(uint8_t tft_cs, uint8_t tft_dc, uint8_t tft_rst, uint8_t tft_lit);
    ~DisplayManager();
    
    // Initialize display
    bool begin();
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

    // Graph Display
    void drawGraph(const char* title, const char* unit,
                   const std::vector<float>& values,
                   const std::vector<time_t>& timestamps,
                   float minVal, float maxVal);

    // Utility Methods
    // TODO Remove the testConnection method
    bool testConnection();
    void setBrightness(uint8_t brightness);
    Adafruit_ST7789* getTFT() { return _tft; }
    void drawHeader(const char* title);
    void drawSeparator(int y);

    // OTA Display
    void showOTAMode(const char* ip);
    void showOTAProgress(int percent, size_t current, size_t total);

    // Battery Display
    void showBatteryInfo(const BatteryStatus& battery);

private:
    Adafruit_ST7789* _tft;

    uint8_t _cs, _dc, _rst, _lit;
    bool _initialized;
    bool _isOn;

    // Internal helper
    
};
