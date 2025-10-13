#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 1

Adafruit_NeoPixel rgbLed(NUM_LEDS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Define some colors
struct RGB {
    uint8_t r, g, b;
};

constexpr RGB COLOR_RED = {255, 0, 0};
constexpr RGB COLOR_GREEN = {0, 255, 0};
constexpr RGB COLOR_BLUE = {0, 0, 255};
constexpr RGB COLOR_OFF = {0, 0, 0};

void setColor(const RGB& color) {
    rgbLed.setPixelColor(0, rgbLed.Color(color.r, color.g, color.b));
    rgbLed.show();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting RGB LED blinking...");

    rgbLed.begin(); // Initialize the NeoPixel library
    rgbLed.show();  // Turn off all LEDs initially
}

void loop() {
    Serial.println("Setting color to RED");
    setColor(COLOR_RED);
    delay(500);

    Serial.println("Setting color to GREEN");
    setColor(COLOR_GREEN);
    delay(500);

    Serial.println("Setting color to BLUE");
    setColor(COLOR_BLUE);
    delay(500);

    Serial.println("Turning off LED");
    setColor(COLOR_OFF);
    delay(500);
}