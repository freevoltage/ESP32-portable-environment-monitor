#include <Arduino.h>
#include <unity.h>
#include "rtc_manager.h"
#include "../test_helpers.h"

RTCManager rtc;

void test_rtc_initialization() {
    TestHelper::printTestHeader("RTC Initialization");
    
    rtc.begin(2024, 12, 15, 14, 30, 0);
    delay(100);
    
    int year, month, day, hour, minute, second;
    rtc.getTimeComponents(year, month, day, hour, minute, second);
    
    Serial.printf("Set time: 2024-12-15 14:30:00\n");
    Serial.printf("Read time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  year, month, day, hour, minute, second);
    
    TEST_ASSERT_EQUAL(2024, year);
    TEST_ASSERT_EQUAL(12, month);
    TEST_ASSERT_EQUAL(15, day);
    TEST_ASSERT_EQUAL(14, hour);
    TEST_ASSERT_EQUAL(30, minute);
    
    TestHelper::printPass("Time set and read correctly");
}

void test_rtc_time_increment() {
    TestHelper::printTestHeader("RTC Time Increment");
    
    rtc.begin(2024, 12, 15, 14, 30, 0);
    delay(100);
    
    Serial.println("Waiting 3 seconds...");
    delay(3000);
    
    int year, month, day, hour, minute, second;
    rtc.getTimeComponents(year, month, day, hour, minute, second);
    
    Serial.printf("Time after 3s: %02d:%02d:%02d\n", hour, minute, second);
    
    TEST_ASSERT_GREATER_OR_EQUAL(3, second);
    
    TestHelper::printPass("Time incremented correctly");
}

void test_rtc_formatted_output() {
    TestHelper::printTestHeader("RTC Formatted Output");
    
    String dateTime = rtc.getFormattedDateTime();
    String time = rtc.getFormattedTime();
    String date = rtc.getFormattedDate();
    
    Serial.printf("DateTime: %s\n", dateTime.c_str());
    Serial.printf("Time: %s\n", time.c_str());
    Serial.printf("Date: %s\n", date.c_str());
    
    TEST_ASSERT_GREATER_THAN(0, dateTime.length());
    TEST_ASSERT_GREATER_THAN(0, time.length());
    TEST_ASSERT_GREATER_THAN(0, date.length());
    
    TestHelper::printPass("All formats working");
}

void test_rtc_timestamp() {
    TestHelper::printTestHeader("RTC Timestamp");
    
    time_t timestamp1 = rtc.getTimestamp();
    delay(1000);
    time_t timestamp2 = rtc.getTimestamp();
    
    Serial.printf("Timestamp 1: %ld\n", timestamp1);
    Serial.printf("Timestamp 2: %ld\n", timestamp2);
    Serial.printf("Difference: %ld seconds\n", timestamp2 - timestamp1);
    
    TEST_ASSERT_GREATER_OR_EQUAL(1, timestamp2 - timestamp1);
    
    TestHelper::printPass("Timestamps incrementing");
}

void setup() {
    Serial.begin(115200);
    TestHelper::waitForSerial();
    
    delay(2000);  // Stabilization delay
    
    UNITY_BEGIN();
    
    RUN_TEST(test_rtc_initialization);
    RUN_TEST(test_rtc_time_increment);
    RUN_TEST(test_rtc_formatted_output);
    RUN_TEST(test_rtc_timestamp);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
