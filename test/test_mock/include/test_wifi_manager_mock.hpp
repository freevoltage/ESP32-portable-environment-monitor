#pragma once
#include <unity.h>
#include "wifi_manager.h"
#include "mock_wifi_manager.h"
#include "mock_control.h"
#include <test_fixture.h>

namespace test_wifi_manager_direct
{
    WiFiManager *wifi = nullptr;

    void setUp() {
        wifi = new WiFiManager();
        resetMockWifi();
    }

    void tearDown() {
        delete wifi;
        wifi = nullptr;
        resetMockWifi();
    }

    void test_initial_state() {
        TEST_ASSERT_NOT_NULL(wifi);
        TEST_ASSERT_FALSE(wifi->isConnected());
    }

    void test_connect_success() {
        bool result = wifi->connect("TestSSID", "TestPass", 10);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_TRUE(wifi->isConnected());
    }

    void test_connect_failure() {
        setMockWifiShouldFail(true);
        bool result = wifi->connect("TestSSID", "TestPass", 10);
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_FALSE(wifi->isConnected());
    }

    void test_disconnect() {
        wifi->connect("TestSSID", "TestPass", 10);
        TEST_ASSERT_TRUE(wifi->isConnected());

        wifi->disconnect();
        TEST_ASSERT_FALSE(wifi->isConnected());
    }

    void test_get_ip_address_connected() {
        wifi->connect("TestSSID", "TestPass", 10);
        String ip = wifi->getIPAddress();
        TEST_ASSERT_EQUAL_STRING("192.168.1.100", ip.c_str());
    }

    void test_get_ip_address_disconnected() {
        String ip = wifi->getIPAddress();
        TEST_ASSERT_EQUAL_STRING("Not connected", ip.c_str());
    }

    void test_get_rssi_connected() {
        wifi->connect("TestSSID", "TestPass", 10);
        int rssi = wifi->getRSSI();
        TEST_ASSERT_EQUAL(-50, rssi);
    }

    void test_get_rssi_disconnected() {
        int rssi = wifi->getRSSI();
        TEST_ASSERT_EQUAL(0, rssi);
    }

    void test_sync_time_ntp_success() {
        wifi->connect("TestSSID", "TestPass", 10);
        bool result = wifi->syncTimeNTP("pool.ntp.org", 3600, 3600);
        TEST_ASSERT_TRUE(result);
    }

    void test_sync_time_ntp_not_connected() {
        bool result = wifi->syncTimeNTP("pool.ntp.org", 3600, 3600);
        TEST_ASSERT_FALSE(result);
    }

    void test_sync_time_ntp_failure() {
        wifi->connect("TestSSID", "TestPass", 10);
        setMockNtpShouldFail(true);
        bool result = wifi->syncTimeNTP("pool.ntp.org", 3600, 3600);
        TEST_ASSERT_FALSE(result);
    }

    void run_tests() {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        RUN_TEST(test_initial_state);
        RUN_TEST(test_connect_success);
        RUN_TEST(test_connect_failure);
        RUN_TEST(test_disconnect);
        RUN_TEST(test_get_ip_address_connected);
        RUN_TEST(test_get_ip_address_disconnected);
        RUN_TEST(test_get_rssi_connected);
        RUN_TEST(test_get_rssi_disconnected);
        RUN_TEST(test_sync_time_ntp_success);
        RUN_TEST(test_sync_time_ntp_not_connected);
        RUN_TEST(test_sync_time_ntp_failure);

        TEST_CONTEXT.clearFixtures();
    }
}
