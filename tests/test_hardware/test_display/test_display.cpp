#include <unity.h>
#include "display.h"
#include "data_structures.h"

// Mock Adafruit_ST7789 class
class MockAdafruit_ST7789 {
public:
    static bool begin_called;
    static bool init_success;
    static uint16_t display_width;
    static uint16_t display_height;
    static String last_text;
    static uint16_t last_color;
    static bool display_cleared;
    
    MockAdafruit_ST7789(uint8_t cs, uint8_t dc, uint8_t rst) {}
    
    bool init() { 
        begin_called = true; 
        return init_success; 
    }
    
    void setRotation(uint8_t rotation) {}
    void fillScreen(uint16_t color) { display_cleared = true; }
    void setTextColor(uint16_t color) { last_color = color; }
    void setTextSize(uint8_t size) {}
    void setCursor(int16_t x, int16_t y) {}
    void print(const String& text) { last_text = text; }
    void println(const String& text) { last_text = text; }
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {}
    
    uint16_t width() { return display_width; }
    uint16_t height() { return display_height; }
    
    static void reset() {
        begin_called = false;
        init_success = true;
        display_width = 240;
        display_height = 320;
        last_text = "";
        last_color = 0;
        display_cleared = false;
    }
};

bool MockAdafruit_ST7789::begin_called = false;
bool MockAdafruit_ST7789::init_success = true;
uint16_t MockAdafruit_ST7789::display_width = 240;
uint16_t MockAdafruit_ST7789::display_height = 320;
String MockAdafruit_ST7789::last_text = "";
uint16_t MockAdafruit_ST7789::last_color = 0;
bool MockAdafruit_ST7789::display_cleared = false;

void setUp(void) {
    MockAdafruit_ST7789::reset();
}

void tearDown(void) {
    // Clean up
}

void test_display_initialization() {
    DisplayManager display;
    display.begin(5, 16, 23, 4); // Mock pin numbers
    
    TEST_ASSERT_TRUE(MockAdafruit_ST7789::begin_called);
    TEST_ASSERT_TRUE(display.isReady());
}

void test_display_initialization_failure() {
    DisplayManager display;
    MockAdafruit_ST7789::init_success = false;
    
    display.begin(5, 16, 23, 4);
    TEST_ASSERT_FALSE(display.isReady());
}

void test_display_clear() {
    DisplayManager display;
    display.begin(5, 16, 23, 4);
    
    display.clear();
    TEST_ASSERT_TRUE(MockAdafruit_ST7789::display_cleared);
}

void test_display_show_reading() {
    DisplayManager display;
    display.begin(5, 16, 23, 4);
    
    SensorReading reading = {25.6, 60.2, 1013.25, 1234567890};
    display.showReading(reading, "2024-01-15 14:30:00");

    TEST_ASSERT_TRUE(MockAdafruit_ST7789::last_text.indexOf("25.6") >= 0);
}

void test_display_show_message() {
    DisplayManager display;
    display.begin(5, 16, 23, 4);
    
    display.showMessage("Test Message");
    TEST_ASSERT_TRUE(MockAdafruit_ST7789::last_text.indexOf("Test Message") >= 0);
}

void test_display_show_error() {
    DisplayManager display;
    display.begin(5, 16, 23, 4);
    
    display.showError("Error occurred");
    TEST_ASSERT_TRUE(MockAdafruit_ST7789::last_text.indexOf("Error occurred") >= 0);
}

void test_display_disconnect_reconnect() {
    DisplayManager display;
    display.begin(5, 16, 23, 4);
    
    display.disconnect();
    display.reconnect();
    // Test that display still works after reconnect
    TEST_ASSERT_TRUE(display.isReady());
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_display_initialization);
    RUN_TEST(test_display_initialization_failure);
    RUN_TEST(test_display_clear);
    RUN_TEST(test_display_show_reading);
    RUN_TEST(test_display_show_message);
    RUN_TEST(test_display_show_error);
    RUN_TEST(test_display_disconnect_reconnect);
    
    return UNITY_END();
}
