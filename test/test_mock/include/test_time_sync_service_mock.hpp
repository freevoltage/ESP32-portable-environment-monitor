#pragma once
#include <unity.h>
#include "time_sync_service.h"
#include "wifi_manager.h"
#include "rtc_manager.h"
#include "connectivity_service.h"
#include "mock_wifi_manager.h"
#include "mock_rtc_manager.h"
#include "mock_time_sync_service.h"
#include "mock_control.h"
#include <test_fixture.h>

namespace test_time_sync_service
{
    WiFiManager *wifi = nullptr;
    RTCManager *rtc = nullptr;
    ConnectivityService *connectivity = nullptr;
    TimeSyncService *timeSync = nullptr;

    void setUp() {
        wifi = new WiFiManager();
        rtc = new RTCManager();
        connectivity = new ConnectivityService(wifi, rtc);
        timeSync = new TimeSyncService();
        resetMockTimeSync();
        resetMockWifi();
        resetMockRtc();
    }

    void tearDown() {
        delete timeSync;
        delete connectivity;
        delete rtc;
        delete wifi;
        timeSync = nullptr;
        connectivity = nullptr;
        rtc = nullptr;
        wifi = nullptr;
        resetMockTimeSync();
        resetMockWifi();
        resetMockRtc();
    }

    void test_initialization() {
        TEST_ASSERT_NOT_NULL(timeSync);
        TEST_ASSERT_FALSE(timeSync->isSyncing());
    }

    void test_begin() {
        bool result = timeSync->begin(rtc, connectivity);
        TEST_ASSERT_TRUE(result);
    }

    void test_default_mode() {
        timeSync->begin(rtc, connectivity);
        SyncMode mode = timeSync->getMode();
        // Default should be BLE_FIRST (from mock initial state)
        TEST_ASSERT_EQUAL(SyncMode::BLE_FIRST, mode);
    }

    void test_set_mode() {
        timeSync->begin(rtc, connectivity);

        timeSync->setMode(SyncMode::WIFI_ONLY);
        TEST_ASSERT_EQUAL(SyncMode::WIFI_ONLY, timeSync->getMode());

        timeSync->setMode(SyncMode::BLE_ONLY);
        TEST_ASSERT_EQUAL(SyncMode::BLE_ONLY, timeSync->getMode());

        timeSync->setMode(SyncMode::OFF);
        TEST_ASSERT_EQUAL(SyncMode::OFF, timeSync->getMode());
    }

    void test_mode_to_string() {
        TEST_ASSERT_EQUAL_STRING("OFF", TimeSyncService::modeToString(SyncMode::OFF));
        TEST_ASSERT_EQUAL_STRING("BLE", TimeSyncService::modeToString(SyncMode::BLE_ONLY));
        TEST_ASSERT_EQUAL_STRING("WiFi", TimeSyncService::modeToString(SyncMode::WIFI_ONLY));
        TEST_ASSERT_EQUAL_STRING("BLE+WiFi", TimeSyncService::modeToString(SyncMode::BLE_FIRST));
        TEST_ASSERT_EQUAL_STRING("WiFi+BLE", TimeSyncService::modeToString(SyncMode::WIFI_FIRST));
    }

    void test_sync_off() {
        timeSync->begin(rtc, connectivity);
        timeSync->setMode(SyncMode::OFF);
        bool result = timeSync->sync();
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_FALSE(timeSync->isSyncing());
    }

    void test_sync_ble_only_success() {
        timeSync->begin(rtc, connectivity);
        timeSync->setMode(SyncMode::BLE_ONLY);
        bool result = timeSync->sync();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL(SyncSource::BLE, timeSync->getLastSyncSource());
    }

    void test_sync_ble_only_failure() {
        timeSync->begin(rtc, connectivity);
        timeSync->setMode(SyncMode::BLE_ONLY);
        setMockBLESyncShouldFail(true);
        bool result = timeSync->sync();
        TEST_ASSERT_FALSE(result);
    }

    void test_sync_wifi_only_success() {
        timeSync->begin(rtc, connectivity);
        wifi->connect("TestSSID", "TestPass", 10);
        timeSync->setMode(SyncMode::WIFI_ONLY);
        bool result = timeSync->sync();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL(SyncSource::WIFI, timeSync->getLastSyncSource());
    }

    void test_sync_wifi_only_failure() {
        timeSync->begin(rtc, connectivity);
        timeSync->setMode(SyncMode::WIFI_ONLY);
        setMockWiFiSyncShouldFail(true);
        bool result = timeSync->sync();
        TEST_ASSERT_FALSE(result);
    }

    void test_sync_ble_first_fallback_to_wifi() {
        timeSync->begin(rtc, connectivity);
        wifi->connect("TestSSID", "TestPass", 10);
        timeSync->setMode(SyncMode::BLE_FIRST);
        setMockBLESyncShouldFail(true);
        bool result = timeSync->sync();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL(SyncSource::WIFI, timeSync->getLastSyncSource());
    }

    void test_sync_wifi_first_fallback_to_ble() {
        timeSync->begin(rtc, connectivity);
        timeSync->setMode(SyncMode::WIFI_FIRST);
        setMockWiFiSyncShouldFail(true);
        bool result = timeSync->sync();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL(SyncSource::BLE, timeSync->getLastSyncSource());
    }

    void test_sync_ble_first_both_fail() {
        timeSync->begin(rtc, connectivity);
        timeSync->setMode(SyncMode::BLE_FIRST);
        setMockBLESyncShouldFail(true);
        setMockWiFiSyncShouldFail(true);
        bool result = timeSync->sync();
        TEST_ASSERT_FALSE(result);
    }

    void test_get_status() {
        timeSync->begin(rtc, connectivity);
        SyncStatus status = timeSync->getStatus();
        TEST_ASSERT_EQUAL(SyncMode::BLE_FIRST, status.mode);
        TEST_ASSERT_FALSE(status.syncInProgress);
    }

    void test_is_not_syncing_by_default() {
        timeSync->begin(rtc, connectivity);
        TEST_ASSERT_FALSE(timeSync->isSyncing());
    }

    void run_tests() {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        RUN_TEST(test_initialization);
        RUN_TEST(test_begin);
        RUN_TEST(test_default_mode);
        RUN_TEST(test_set_mode);
        RUN_TEST(test_mode_to_string);
        RUN_TEST(test_sync_off);
        RUN_TEST(test_sync_ble_only_success);
        RUN_TEST(test_sync_ble_only_failure);
        RUN_TEST(test_sync_wifi_only_success);
        RUN_TEST(test_sync_wifi_only_failure);
        RUN_TEST(test_sync_ble_first_fallback_to_wifi);
        RUN_TEST(test_sync_wifi_first_fallback_to_ble);
        RUN_TEST(test_sync_ble_first_both_fail);
        RUN_TEST(test_get_status);
        RUN_TEST(test_is_not_syncing_by_default);

        TEST_CONTEXT.clearFixtures();
    }
}
