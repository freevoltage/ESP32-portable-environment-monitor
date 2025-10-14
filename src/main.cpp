
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>


#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "FS.h"
//#include "SD.h" // When adding the SD Library it doesnt compile anymore

#include "main.h"


RTC_DATA_ATTR int bootCount = 0;  // Variable stored in RTC memory

#define SEALEVELPRESSURE_HPA (1013.25)

// Define FILE_APPEND explicitly to avoid ambiguity with SD.h
#define FILE_APPEND (0X01 | 0X02 | 0X10 | 0X04) // O_READ | O_WRITE | O_CREAT | O_APPEND

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

Adafruit_BME280 bme;

// NEXT STEPS
// [ ] Read sensor data and save them to the sd card.
// [ ] Figure out how many sensor readings I can read before I need to save them to the SD card (save some energy)
// [ ] It would be very nice to have an error system using the neopixel LED. If an error occurs just turn it red.


void setup(){

    Serial.begin(115200);
    while(!Serial);
    Serial.print("Serial Connection Established!");

    tft.init(135, 240);
    tft.fillScreen(ST77XX_BLACK);

    Wire.begin(SDA, SCL);
    unsigned bme_status;
    bme_status = bme.begin(BME280_ADDRESS, &Wire);

    if(!bme_status){
        Serial.print("Could not connect to the BME280 sensor!");
    }
    /*
    if(!SD.begin(SD_CS)){
        Serial.println("Card Mount Failed");
        return;
    }
    Serial.println("SD Card initialized.");
    */
}

void loop(){
    float temperature = bme.readTemperature();
    float pressure = bme.readPressure() / 100.0F;
    float humidity = bme.readHumidity();
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    printValues();
    displaySensorData();
    //writeSensorDataToSD(temperature, pressure, humidity, altitude);
    delay(1000);
}

void displaySensorData(){
    tft.setRotation(1);
    tft.setTextWrap(true);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);

    tft.print("Temp: ");
    tft.print(bme.readTemperature());
    tft.println(" C");

    tft.print("Pres: ");
    tft.print(bme.readPressure() / 100.0F);
    tft.println(" hPa");

    tft.print("Hum: ");
    tft.print(bme.readHumidity());
    tft.println(" %");

    tft.print("Alt: ");
    tft.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    tft.println(" m");
}


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

/*
void writeSensorDataToSD(float temperature, float pressure, float humidity, float altitude) {
    SDLib::File dataFile = SD.open("/datalog.csv", (uint8_t)FILE_APPEND);
    if (dataFile) {
        String dataString = "";
        dataString += String(temperature);
        dataString += ",";
        dataString += String(pressure);
        dataString += ",";
        dataString += String(humidity);
        dataString += ",";
        dataString += String(altitude);
        dataFile.println(dataString);
        dataFile.close();
        Serial.println("Data saved to datalog.csv");
    } else {
        Serial.println("Error opening datalog.csv");
    }
}
    */
