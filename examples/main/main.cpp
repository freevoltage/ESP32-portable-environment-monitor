
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>


#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "main.h"
#include <I2CScanner.h>

//void setup();
//void loop();

#define SEALEVELPRESSURE_HPA (1013.25)

//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//Adafruit_BME280 bme;


void setup(){

    Serial.begin(115200);
    while(!Serial);
    Serial.print("Serial Connection Established!");

    //I2CScanner::begin(Wire, SDA, SCL);
    //I2CScanner::scan(Wire);

    //tft.init(135, 240);
    //tft.fillScreen(ST77XX_BLACK);

    //Wire.begin();
    //Wire.begin(SDA, SCL);
    //Wire.begin(19, 18);
    unsigned bme_status;
    //bme_status = bme.begin(BME280_ADDRESS, &Wire);

    if(!bme_status){
        Serial.print("Could not connect to the BME280 sensor!");
    }
}

void loop(){
    //displayText();
}

/*
void displayText(){
    tft.setTextWrap(true);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 30);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(5);
    tft.println("Hello World");
}
*/

/*
void printValues() {
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}
    */