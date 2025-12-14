#include <Arduino.h>
#include <sensor.h>

SensorManager sensor;
SensorReading reading;

void setup(){
    Serial.begin(115200);
    while(!Serial);

    sensor.begin();
}

void loop(){
    reading = sensor.getReading();
    sensor.printReading(reading);
    delay(1000);
}
