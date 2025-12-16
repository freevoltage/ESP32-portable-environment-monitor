#include <unity.h>
#include <Arduino.h>
#include "rtc_manager.h"
#include <logger.h>
#include <test_fixture.h>

RTCManager rtcManager;

// === Basic Initialization Tests ===
void test_initial_state() {
    TEST_ASSERT_FALSE(rtcManager.isReady());
    TEST_ASSERT_FALSE(rtcManager.isTimeSet());
    TEST_ASSERT_EQUAL(0, rtcManager.getLastSyncTime());
}

void test_begin() {
    TEST_ASSERT_TRUE(rtcManager.begin());
    TEST_ASSERT_TRUE(rtcManager.isReady());
}

// === Time Setting and Getting Tests ===
void test_set_and_get_time() {
    rtcManager.begin();
    
    // Set known time: March 15, 2024, 14:30:45
    TEST_ASSERT_TRUE(rtcManager.setTime(2024, 3, 15, 14, 30, 45));
    TEST_ASSERT_TRUE(rtcManager.isTimeSet());
    
    // Get time back and verify it's reasonable
    time_t epochTime = rtcManager.getEpochTime();
    TEST_ASSERT_GREATER_THAN(1640995200, epochTime); // After Jan 1, 2022
    
    // Check formatted time contains expected elements
    String formatted = rtcManager.getFormattedTime();
    LOG_INFO("%s", formatted.c_str());
    TEST_ASSERT_TRUE(formatted.indexOf("2024") >= 0);
    TEST_ASSERT_TRUE(formatted.indexOf("15") >= 0);
}

void test_time_struct() {
    rtcManager.begin();
    rtcManager.setTime(2024, 3, 15, 14, 30, 45);
    
    tm timeStruct = rtcManager.getTimeStruct();
    TEST_ASSERT_EQUAL(2024 - 1900, timeStruct.tm_year); // tm_year is years since 1900
    TEST_ASSERT_EQUAL(3 - 1, timeStruct.tm_mon);        // tm_mon is 0-11
    TEST_ASSERT_EQUAL(15, timeStruct.tm_mday);
    TEST_ASSERT_EQUAL(14, timeStruct.tm_hour);
    TEST_ASSERT_EQUAL(30, timeStruct.tm_min);
    TEST_ASSERT_EQUAL(45, timeStruct.tm_sec);
}

// === NTP Sync Management Tests ===
void test_sync_time_management() {
    rtcManager.begin();
    
    // Initially needs sync
    TEST_ASSERT_TRUE(rtcManager.needsTimeSync(24));
    
    // Set sync time
    time_t syncTime = 1710000000; // Some valid timestamp
    rtcManager.setLastSyncTime(syncTime);
    TEST_ASSERT_EQUAL(syncTime, rtcManager.getLastSyncTime());
}

void test_needs_sync_logic() {
    rtcManager.begin();
    rtcManager.setTime(2024, 3, 15, 14, 30, 45);
    
    // Set sync time to current time
    time_t currentTime = rtcManager.getEpochTime();
    rtcManager.setLastSyncTime(currentTime);
    
    // Should not need sync immediately
    TEST_ASSERT_FALSE(rtcManager.needsTimeSync(24));
    
    // Should need sync with very short interval
    TEST_ASSERT_TRUE(rtcManager.needsTimeSync(0)); // 0 hours = immediate
}

// === Edge Cases and Error Handling ===
void test_invalid_time_detection() {
    rtcManager.begin();
    
    // Set very old time (should be considered invalid)
    rtcManager.setTime(1970, 1, 1, 0, 0, 0);
    TEST_ASSERT_FALSE(rtcManager.isTimeSet());
    
    // Set reasonable time
    rtcManager.setTime(2024, 1, 1, 0, 0, 0);
    TEST_ASSERT_TRUE(rtcManager.isTimeSet());
}

void test_custom_format() {
    rtcManager.begin();
    rtcManager.setTime(2024, 3, 15, 14, 30, 45);
    
    String customFormat = rtcManager.getFormattedTime("%H:%M");
    TEST_ASSERT_TRUE(customFormat.indexOf("14:30") >= 0);
}

// === Integration Test ===
void test_typical_usage_flow() {
    // Simulate typical device startup and usage
    TEST_ASSERT_TRUE(rtcManager.begin());
    TEST_ASSERT_TRUE(rtcManager.needsTimeSync()); // Initial state
    
    // Simulate setting time from NTP
    rtcManager.setTime(2024, 3, 15, 14, 30, 45);
    rtcManager.setLastSyncTime(rtcManager.getEpochTime());
    
    TEST_ASSERT_TRUE(rtcManager.isTimeSet());
    TEST_ASSERT_FALSE(rtcManager.needsTimeSync(24));
    TEST_ASSERT_GREATER_THAN(0, rtcManager.getEpochTime());
}


/// @brief 
/// ESP32Time.h contains:
///		tm getTimeStruct();
///		String getTime(String format);
///		
///		String getTime();
///		String getDateTime(bool mode = false);
///		String getTimeDate(bool mode = false);
///		String getDate(bool mode = false);
///		String getAmPm(bool lowercase = false);
void printRTCFormats(){
    time_t epoch = rtcManager.getEpochTime();
    LOG_INFO("Epoch: %d", epoch);

    String formatted = rtcManager.getFormattedTime();
    LOG_INFO("Formatted: %s", formatted.c_str());

    struct tm timeinfo = rtcManager.getTimeStruct();

        // Format the tm struct into a string first
    char timeStr[64];
    if (strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &timeinfo) > 0) {
        LOG_INFO("Time struct: %s", timeStr);
    } else {
        LOG_ERROR("Failed to format time struct");
    }
}

namespace test_rtc{

    void setUp(void) {
        // Create fresh instance for each test
        rtcManager = RTCManager();
    }
    
    void tearDown(void) {
        // Clean up after each test
    }
    
    void run_tets(){
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        LOG_INFO("\n[RUN TEST] RTC:\n");

        // Basic functionality
        RUN_TEST(test_initial_state);
        RUN_TEST(test_begin);
        
        // Core time operations
        RUN_TEST(test_set_and_get_time);
        RUN_TEST(test_time_struct);
        RUN_TEST(test_custom_format);
        
        // NTP sync management
        RUN_TEST(test_sync_time_management);
        RUN_TEST(test_needs_sync_logic);
        
        // Edge cases
        RUN_TEST(test_invalid_time_detection);
        
        // Integration
        RUN_TEST(test_typical_usage_flow);

        //printRTCFormats();

        LOG_INFO("Print RTC Formats directly from ESP32Time");
        rtcManager.printRTCFormats();
    }
}