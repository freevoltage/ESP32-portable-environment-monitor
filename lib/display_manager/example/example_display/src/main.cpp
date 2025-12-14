#include <Arduino.h>
#include <display_manager.h>
#include <data_structures.h>
#include <config.h>

DisplayManager display;

uint32_t dummy_temperature;

void testSequence(){
    SensorReading dummyReading = SensorReading(23.0,50, 500, 1100);

    Serial.println("=== Displaying Dummy Sensor Reading ===");
    Serial.printf("Temperature: %.1f°C\n", dummyReading.temperature);
    Serial.printf("Pressure: %.0f hPa\n", dummyReading.pressure);
    Serial.printf("Humidity: %.0f%%\n", dummyReading.humidity);
    //Serial.printf("Altitude: %.0f m\n", dummyReading.altitude);
    Serial.println("========================================");
    
    display.showReading(dummyReading);
}

void setup(){
    Serial.begin(115200);
    while(!Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    display.begin(TFT_CS, TFT_DC, TFT_RST, TFT_LIT);
}

void loop(){
    testSequence();
    delay(1000);
}