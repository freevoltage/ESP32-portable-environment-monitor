/* 
    Display Manager Class to support the 
    Adafruit 1.14" 240x135 Color TFT Breakout LCD Display
    See:
    https://learn.adafruit.com/adafruit-1-14-240x135-color-tft-breakout
*/

#include "display.h"
#include "config.h"

DisplayManager::DisplayManager() : tft(nullptr), _initialized(false) {
    // Empty Constructor
}

DisplayManager::~DisplayManager(){
    if (tft) {
        delete tft;
        tft = nullptr;
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

    tft = new Adafruit_ST7789(_cs, _dc, _rst);

    tft->init(135, 240);
    tft-> setRotation(3);
    clear();

    digitalWrite(_lit, HIGH);
    _initialized = true;
    Serial.println("Display initialized");
}

void DisplayManager::disconnect() {
    if (!isReady()) return;

    Serial.println("Powering off display...");
    
    clear();

    // Send display to sleep mode
    tft->sendCommand(ST77XX_DISPOFF);    // Display OFF
    delay(50);
    tft->enableSleep(true);
    delay(120);

    // Turn off Backlight
    digitalWrite(_lit, LOW);

    
    delay(120);
    digitalWrite(TFT_LIT, LOW);

    // Note: Don't call SPI.end() to keep the bus available for the SD card
    _initialized = false;
    Serial.println("Display powered off");
}

void DisplayManager::reconnect(){
    if (!tft) return;
    
    Serial.println("Reconnecting display...");
    
    // Wake up display
    tft->sendCommand(ST77XX_SLPOUT);     // Sleep OUT
    delay(120);
    tft->enableSleep(false);    // Display ON
    
    // Turn on backlight
    digitalWrite(_lit, HIGH);
    
    _initialized = true;
    Serial.println("Display reconnected");
}

void DisplayManager::clear() {
    if(!isReady()) return;
    tft->fillScreen(ST77XX_BLACK);
}

void DisplayManager::showReading(const SensorReading &reading, const String &dateTime) {
    if (!isReady()) return;
    
    clear();
    tft->setCursor(0, 0);
    
    // Date and time
    tft->setTextColor(ST77XX_CYAN);
    tft->setTextSize(1);
    tft->println(dateTime);
    tft->println();
    
    // Sensor readings
    tft->setTextColor(ST77XX_WHITE);
    tft->setTextSize(2);
    
    tft->print("T: ");
    tft->print(reading.temperature, 1);
    tft->println(" C");
    
    tft->print("P: ");
    tft->print(reading.pressure, 0);
    tft->println(" hPa");
    
    tft->print("H: ");
    tft->print(reading.humidity, 0);
    tft->println(" %");
    
    tft->print("A: ");
    tft->print(reading.altitude, 0);
    tft->println(" m");
}

void DisplayManager::showBootCount(int bootCount) {
    if (!isReady()) return;

    tft->setCursor(0, 120);
    tft->setTextSize(1);
    tft->setTextColor(ST77XX_YELLOW);
    tft->print("Boot #");
    tft->println(bootCount);
}

void DisplayManager::showError(const String &message) {
    if (!isReady()) return;

    clear();
    tft->setCursor(0, 0);
    tft->setTextColor(ST77XX_RED);
    tft->setTextSize(2);
    tft->println("ERROR:");
    tft->println();
    tft->setTextSize(1);
    tft->println(message);
}

void DisplayManager::showStatus(const String &message) {
    if (!isReady()) return;

    tft->setCursor(0, 110);
    tft->setTextColor(ST77XX_GREEN);
    tft->setTextSize(1);
    tft->println(message);
}

bool DisplayManager::isReady()
{
    // Check if the Display is initialized and that the tft is not a Nullpointer.
    return (_initialized && tft != nullptr);
}
