#include <unity.h>
#include "rtc_manager.h"
#include <time.h>

// Mock system time functions
class MockTime {
public:
    static time_t mock_time;
    static struct tm mock_tm;
    static bool time_set;
    
    static void reset() {
        mock_time = 1704067800; // 2024-01-01 00:00:00
        mock_tm = {0, 0, 0, 1, 0, 124, 1, 0, 0}; // 2024-01-01
        time_set = false;
    }
    
    static time_t time(time_t* t) {
        if (t) *t = mock_time;
        return mock_time;
    }
    
    static struct tm* localtime(const time_t* timep) {
        return &mock_tm;
    }
    
    static void setTime(int year, int month, int day, int hour, int min, int sec) {
        mock_tm.tm_year = year - 1900;
        mock_tm.tm_mon = month - 1;
        mock_tm.tm_mday = day;
        mock_tm.tm_hour = hour;
        mock_tm.tm_min = min;
        mock_tm.tm_sec = sec;
        mock_time = mktime(&mock_tm);
        time_set = true;
    }
};

time_t MockTime::mock_time = 1704067800;
struct tm MockTime::mock_tm = {0, 0, 0, 1, 0, 124, 1, 0, 0};
bool MockTime::time_set = false;

void setUp(void) {
    MockTime::reset();
}

void tearDown(void) {
    // Clean up
}

void test_rtc_initialization() {
    RTCManager rtc;
    rtc.begin(2024, 1, 15, 14, 30, 0);
    
    TEST_ASSERT_TRUE(MockTime::time_set);
    TEST_ASSERT_TRUE(rtc.isInitialized());
}

void test_rtc_get_formatted_datetime() {
    RTCManager rtc;
    MockTime::setTime(2024, 1, 15, 14, 30, 45);
    
    String datetime = rtc.getFormattedDateTime();
    TEST_ASSERT_TRUE(datetime.indexOf("2024") >= 0);
    TEST_ASSERT_TRUE(datetime.indexOf("01") >= 0);
    TEST_ASSERT_TRUE(datetime.indexOf("15") >= 0);
}

void test_rtc_get_formatted_time() {
    RTCManager rtc;
    MockTime::setTime(2024, 1, 15, 14, 30, 45);
    
    String time_str = rtc.getFormattedTime();
    TEST_ASSERT_TRUE(time_str.indexOf("14") >= 0);
    TEST_ASSERT_TRUE(time_str.indexOf("30") >= 0);
}

void test_rtc_get_formatted_date() {
    RTCManager rtc;
    MockTime::setTime(2024, 1, 15, 14, 30, 45);
    
    String date_str = rtc.getFormattedDate();
    TEST_ASSERT_TRUE(date_str.indexOf("2024") >= 0);
    TEST_ASSERT_TRUE(date_str.indexOf("01") >= 0);
    TEST_ASSERT_TRUE(date_str.indexOf("15") >= 0);
}

void test_rtc_time_components() {
    RTCManager rtc;
    MockTime::setTime(2024, 1, 15, 14, 30, 45);
    
    int year, month, day, hour, minute, second;
    rtc.getTimeComponents(year, month, day, hour, minute, second);
    
    TEST_ASSERT_EQUAL_INT(2024, year);
    TEST_ASSERT_EQUAL_INT(1, month);
    TEST_ASSERT_EQUAL_INT(15, day);
    TEST_ASSERT_EQUAL_INT(14, hour);
    TEST_ASSERT_EQUAL_INT(30, minute);
    TEST_ASSERT_EQUAL_INT(45, second);
}

void test_rtc_ntp_sync_tracking() {
    RTCManager rtc;
    time_t sync_time = 1704067800; // Test timestamp
    
    rtc.setLastSyncTime(sync_time);
    TEST_ASSERT_EQUAL_INT(sync_time, rtc.getLastSyncTime());
}

void test_rtc_needs_time_sync() {
    RTCManager rtc;
    time_t old_sync = 1704067800; // Old sync time
    MockTime::mock_time = 1704067800 + (25 * 3600); // 25 hours later
    
    rtc.setLastSyncTime(old_sync);
    TEST_ASSERT_TRUE(rtc.needsTimeSync(24)); // Should need sync after 24h
    TEST_ASSERT_FALSE(rtc.needsTimeSync(48)); // Shouldn't need sync within 48h
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_rtc_initialization);
    RUN_TEST(test_rtc_get_formatted_datetime);
    RUN_TEST(test_rtc_get_formatted_time);
    RUN_TEST(test_rtc_get_formatted_date);
    RUN_TEST(test_rtc_time_components);
    RUN_TEST(test_rtc_ntp_sync_tracking);
    RUN_TEST(test_rtc_needs_time_sync);
    
    return UNITY_END();
}
