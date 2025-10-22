/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2652

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface. The device's I2C address is either 0x76 or 0x77.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  See the LICENSE file for details.
 ***************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "main.h"
#include <config.h>
#include <storage.h>
#include <sensor.h>

#define SD_MOSI  22   // Master Out Slave In
#define SD_MISO  23   // Master In Slave Out  
#define SD_SCK   21   // Serial Clock

#define SEALEVELPRESSURE_HPA (1013.25)

unsigned long delayTime;

StorageManager storage;
SensorManager sensor;


void setup() {
    Serial.begin(115200);
    while(!Serial);
    delay(100);
    Serial.println(F("BME280 test"));

    unsigned bme_status;

    // Init BME280
    sensor.begin();
    
    // Init SD Card
    storage.begin(); // Attention the SD Card begin function is not implemented yet

    storage.getCardInfo();
}


void loop() {
    SensorReading reading = sensor.takeReading();
    storage.writeReading(reading, " ");
    sensor.printReading(reading);
    delay(1000);
}