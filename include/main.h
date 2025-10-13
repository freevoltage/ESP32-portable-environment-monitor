#pragma once

// PIN DEFINITIONS

#define TFT_CS  5
#define TFT_RST 6
#define TFT_DC  7

#define SD_CS 13

// Function Declarations

void printValues();
void displaySensorData();
void writeSensorDataToSD(float temperature, float pressure, float humidity, float altitude);
