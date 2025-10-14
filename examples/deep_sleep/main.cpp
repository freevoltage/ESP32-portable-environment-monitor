/* 
This example sends the MCU into deep sleep after 10 seconds. 

*/


#include <Arduino.h>

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  10          // Time ESP32 will go to sleep (in seconds)
// This is the time the ESP stays in deep_sleep.
//Its not the time the ESP which it stays awake

RTC_DATA_ATTR int bootCount = 0;  // Variable stored in RTC memory

void blink_led(uint8_t times, uint32_t time){
    for (int i=0; i<times; i++){
        digitalWrite(LED_BUILTIN, HIGH);
        delay(time);
        digitalWrite(LED_BUILTIN, LOW);
        delay(time);
    }
}

void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER: 
            Serial.println("Wakeup caused by timer"); 
            break;
        case ESP_SLEEP_WAKEUP_EXT0: 
            Serial.println("Wakeup caused by external signal using RTC_IO"); 
            break;
        case ESP_SLEEP_WAKEUP_EXT1: 
            Serial.println("Wakeup caused by external signal using RTC_CNTL"); 
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: 
            Serial.println("Wakeup caused by touchpad"); 
            break;
        case ESP_SLEEP_WAKEUP_ULP: 
            Serial.println("Wakeup caused by ULP program"); 
            break;
        default: 
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); 
            break;
    }


}

void setup() {
    Serial.begin(115200);
    delay(1000);  // Give time for serial to initialize
    blink_led(3, 300);

    pinMode(LED_BUILTIN, OUTPUT);
    
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    blink_led(bootCount, 500);

    // Print the wakeup reason for ESP32
    printWakeupReason();

    // Configure the timer to wake up after TIME_TO_SLEEP seconds
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for " + String(TIME_TO_SLEEP) + " Seconds");

    // Do your work here (read sensor, save to SD, etc.)
    Serial.println("Going to sleep now");
    Serial.flush(); 
    
    // Enter deep sleep
    esp_deep_sleep_start();
}

void loop() {
    // This will never be reached because we enter deep sleep in setup()
}
