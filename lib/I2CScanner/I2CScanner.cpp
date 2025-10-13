#include "I2CScanner.h"

void I2CScanner::begin(TwoWire &wire, int sda, int scl) {
    wire.begin(sda, scl);
    Serial.println("\nI2C Scanner initialized");
    Serial.print("SDA Pin: "); Serial.print(sda);
    Serial.print(", SCL Pin: "); Serial.println(scl);
}

void I2CScanner::scan(TwoWire &wire) {
    byte error, address;
    int nDevices = 0;
    
    Serial.println("Scanning I2C bus...");
    for (address = 1; address < 127; address++) {
        wire.beginTransmission(address);
        error = wire.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
            nDevices++;
        } 
        else if (error == 4) {
            Serial.print("Unknown error at 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("Scan complete\n");
}