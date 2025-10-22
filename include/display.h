#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include "sensor.h"

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Initialize display
    void begin(uint8_t tft_cs, uint8_t tft_dc, uint8_t tft_rst, uint8_t tft_lit);

    void disconnect();
    void reconnect();  // Not sure if I need this actually

    // Display Operations
    void clear();

    // TODO I might want to add or change this functions later.
    void showReading(const SensorReading &reading, const String &dateTime);
    void showBootCount(int bootCount);
    void showError(const String &message);
    void showStatus(const String &message);

private:
    Adafruit_ST7789* tft;

    uint8_t _cs, _dc, _rst, _lit;
    bool _initialized;

    // Internal helper
    bool isReady();
};

#endif
