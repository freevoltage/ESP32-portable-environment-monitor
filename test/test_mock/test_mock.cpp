#include <unity.h>
#include <test_fixture.h>

#ifdef MOCK
#include <ctime>
static unsigned long _mock_millis = 1000;
unsigned long millis() { return _mock_millis; }
void delay(unsigned long ms) { _mock_millis += ms; }
void pinMode(int, int) {}
void digitalWrite(int, int) {}
#endif

#include "test_data_service_mock.hpp"
#include "test_connectivity_service_mock.hpp"
#include "test_battery_manager_mock.hpp"
#include "test_wifi_manager_mock.hpp"
#include "test_time_sync_service_mock.hpp"

void setUp() {
    TEST_CONTEXT.runSetUp();
}

void tearDown() {
    TEST_CONTEXT.runTearDown();
}

#ifdef MOCK
int main() {
    UNITY_BEGIN();
    test_data_service::run_tests();
    test_connectivity_service::run_tests();
    test_battery_manager::run_tests();
    test_wifi_manager_direct::run_tests();
    test_time_sync_service::run_tests();
    UNITY_END();
    return 0;
}
#endif
