#include <unity.h>
#include "connectivity_service.h"
#include "wifi_manager.h"
#include "rtc_manager.h"
#include "logger.h"
#include "test_utils.h"
#include <test_fixture.h>

#include "mock_wifi_manager.h"
#include "mock_rtc_manager.h"
#include "mock_control.h"

namespace test_connectivity_service
{
    WiFiManager *wifi = nullptr;
    RTCManager *rtc = nullptr;
    ConnectivityService *connectivity = nullptr;

    void test_initialization()
    {
        TEST_ASSERT_NOT_NULL(connectivity);
        TEST_ASSERT_EQUAL(ConnectivityStatus::DISCONNECTED, connectivity->getStatus());
        TEST_ASSERT_FALSE(connectivity->isConnected());
    }

    void test_connect_success()
    {
        bool result = connectivity->connect();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_TRUE(connectivity->isConnected());
        TEST_ASSERT_EQUAL(ConnectivityStatus::CONNECTED, connectivity->getStatus());
    }

    void test_connect_wifi_fail()
    {
        setMockWifiShouldFail(true);
        bool result = connectivity->connect();
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_FALSE(connectivity->isConnected());
        TEST_ASSERT_EQUAL(ConnectivityStatus::DISCONNECTED, connectivity->getStatus());
        setMockWifiShouldFail(false);
    }

    void test_connect_already_connected()
    {
        TEST_ASSERT_TRUE(connectivity->connect());
        bool result = connectivity->connect();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_TRUE(connectivity->isConnected());
    }

    void test_disconnect()
    {
        TEST_ASSERT_TRUE(connectivity->connect());
        TEST_ASSERT_TRUE(connectivity->isConnected());
        connectivity->disconnect();
        TEST_ASSERT_FALSE(connectivity->isConnected());
        TEST_ASSERT_EQUAL(ConnectivityStatus::DISCONNECTED, connectivity->getStatus());
    }

    void test_ensure_time_sync_success()
    {
        bool result = connectivity->ensureTimeSync();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_TRUE(connectivity->isConnected());
        TEST_ASSERT_EQUAL(ConnectivityStatus::SYNC_COMPLETE, connectivity->getStatus());
        TEST_ASSERT_GREATER_THAN(0, connectivity->getTimeSyncLastSync());
    }

    void test_ensure_time_sync_no_wifi()
    {
        setMockWifiShouldFail(true);
        bool result = connectivity->ensureTimeSync();
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_EQUAL(ConnectivityStatus::DISCONNECTED, connectivity->getStatus());
        setMockWifiShouldFail(false);
    }

    void test_ensure_time_sync_ntp_fail()
    {
        setMockNtpShouldFail(true);
        bool result = connectivity->ensureTimeSync();
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_EQUAL(ConnectivityStatus::CONNECTED, connectivity->getStatus());
        setMockNtpShouldFail(false);
    }

    void test_is_time_sync_required_initial()
    {
        TEST_ASSERT_TRUE(connectivity->isTimeSyncRequired());
    }

    void test_is_time_sync_required_after_sync()
    {
        connectivity->ensureTimeSync();
        TEST_ASSERT_FALSE(connectivity->isTimeSyncRequired());
    }

    void test_sync_time_not_needed()
    {
        connectivity->ensureTimeSync();
        bool result = connectivity->syncTimeIfNeeded();
        TEST_ASSERT_TRUE(result);
    }

    void test_get_status()
    {
        TEST_ASSERT_EQUAL(ConnectivityStatus::DISCONNECTED, connectivity->getStatus());
        connectivity->connect();
        TEST_ASSERT_EQUAL(ConnectivityStatus::CONNECTED, connectivity->getStatus());
    }

    void setUp()
    {
        wifi = new WiFiManager();
        rtc = new RTCManager();
        rtc->begin();
        connectivity = new ConnectivityService(wifi, rtc);
    }

    void tearDown()
    {
        delete connectivity;
        delete rtc;
        delete wifi;

        connectivity = nullptr;
        rtc = nullptr;
        wifi = nullptr;

        resetMockWifi();
        resetMockRtc();
    }

    void run_tests()
    {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        RUN_TEST(test_initialization);
        RUN_TEST(test_connect_success);
        RUN_TEST(test_connect_wifi_fail);
        RUN_TEST(test_connect_already_connected);
        RUN_TEST(test_disconnect);
        RUN_TEST(test_ensure_time_sync_success);
        RUN_TEST(test_ensure_time_sync_no_wifi);
        RUN_TEST(test_ensure_time_sync_ntp_fail);
        RUN_TEST(test_is_time_sync_required_initial);
        RUN_TEST(test_is_time_sync_required_after_sync);
        RUN_TEST(test_sync_time_not_needed);
        RUN_TEST(test_get_status);

        TEST_CONTEXT.clearFixtures();
    }
}
