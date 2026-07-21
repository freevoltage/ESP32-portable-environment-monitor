#pragma once
#include <unity.h>
#include "battery_manager.h"
#include "data_structures.h"
#include "mock_control.h"
#include <test_fixture.h>

#include "mock_battery_manager.h"

namespace test_battery_manager
{
    BatteryManager *battery = nullptr;

    void setUp() {
        battery = new BatteryManager();
        resetMockBattery();
    }

    void tearDown() {
        delete battery;
        battery = nullptr;
        resetMockBattery();
    }

    void test_initialization() {
        TEST_ASSERT_NOT_NULL(battery);
        TEST_ASSERT_FALSE(battery->isReady());
    }

    void test_begin_success() {
        bool result = battery->begin();
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_TRUE(battery->isReady());
    }

    void test_begin_failure() {
        setMockBatteryShouldFail(true);
        bool result = battery->begin();
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_FALSE(battery->isReady());
    }

    void test_get_status_before_begin() {
        BatteryStatus status = battery->getStatus();
        TEST_ASSERT_FALSE(status.isValid);
    }

    void test_get_status_after_begin() {
        battery->begin();
        BatteryStatus status = battery->getStatus();
        TEST_ASSERT_TRUE(status.isValid);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.15f, status.voltage);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 85.0f, status.percent);
    }

    void test_get_status_voltage_range() {
        battery->begin();

        setMockBatteryVoltage(3.20f);
        BatteryStatus status = battery->getStatus();
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.20f, status.voltage);

        setMockBatteryVoltage(4.20f);
        status = battery->getStatus();
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.20f, status.voltage);
    }

    void test_get_status_percent_range() {
        battery->begin();

        setMockBatteryPercent(0.0f);
        BatteryStatus status = battery->getStatus();
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, status.percent);

        setMockBatteryPercent(100.0f);
        status = battery->getStatus();
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, status.percent);
    }

    void test_get_status_failure() {
        battery->begin();
        setMockBatteryShouldFail(true);
        BatteryStatus status = battery->getStatus();
        TEST_ASSERT_FALSE(status.isValid);
    }

    void test_enable_disable_i2c_power() {
        battery->begin();
        battery->enableI2CPower();
        battery->disableI2CPower();
        // No assertion needed — just verify no crash
        TEST_ASSERT_TRUE(true);
    }

    void run_tests() {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        RUN_TEST(test_initialization);
        RUN_TEST(test_begin_success);
        RUN_TEST(test_begin_failure);
        RUN_TEST(test_get_status_before_begin);
        RUN_TEST(test_get_status_after_begin);
        RUN_TEST(test_get_status_voltage_range);
        RUN_TEST(test_get_status_percent_range);
        RUN_TEST(test_get_status_failure);
        RUN_TEST(test_enable_disable_i2c_power);

        TEST_CONTEXT.clearFixtures();
    }
}
