#pragma once
#include <Arduino.h>
#include <Wire.h>

class I2CScanner {
public:
    static void begin(TwoWire &wire = Wire, int sda = SDA, int scl = SCL);
    static void scan(TwoWire &wire = Wire);
};