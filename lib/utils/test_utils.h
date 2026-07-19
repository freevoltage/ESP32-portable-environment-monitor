#pragma once
#include "data_structures.h"
#include <vector>

void assertReadingsEqual(const SensorReading &expected, const SensorReading &actual);
void assertFloatsEqual(float float1, float float2);
void printReading(const char *label, const SensorReading &r);
void printAllReadings(const char *label, std::vector<SensorReading> readings);