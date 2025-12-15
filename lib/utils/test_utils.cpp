#include "test_utils.h"
#include "unity.h"
#include "logger.h"

void printReading(const char *label, const SensorReading &r)
{
    LOG_DEBUG("%s: ts=%lld, temp=%.2f, hum=%.2f%% press= %.2f",
              label, r.timestamp, r.temperature, r.humidity, r.pressure);
}

void printAllReadings(const char *label, std::vector<SensorReading> readings)
{
    for (int i = 0; i < readings.size(); i++)
        printReading(label, readings[i]);
}

void assertReadingsEqual(const SensorReading &expected, const SensorReading &actual)
{
    printReading("AssertReadingsEqual: Expected", expected);
    printReading("AssertReadingsEqual: Actual", actual);

    assertFloatsEqual(expected.temperature, actual.temperature);
    assertFloatsEqual(expected.humidity, actual.humidity);
    assertFloatsEqual(expected.pressure, actual.pressure);

    TEST_ASSERT_EQUAL(expected.timestamp, actual.timestamp);
}

void assertFloatsEqual(float float1, float float2)
{
    // Convert floats to integers (multiply by 100 for 2 decimal precision)
    TEST_ASSERT_EQUAL_INT32(
        (int32_t)roundf(float1 * 100),
        (int32_t)roundf(float2 * 100));
}
