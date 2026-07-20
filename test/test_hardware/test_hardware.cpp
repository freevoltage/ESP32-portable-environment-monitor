#undef MOCK
#include <unity.h>
#include <test_runner.h>
#include <test_fixture.h>

#include "test_storage.hpp"
#include "test_sensor.hpp"
#include "test_display.hpp"
#include "test_rtc.hpp"
#include "test_hiking_station.hpp"

void setUp() {
    TEST_CONTEXT.runSetUp();  // Dispatch to active namespace
}

void tearDown() {
    TEST_CONTEXT.runTearDown();  // Dispatch to active namespace
}

void setup(){
    Serial.begin(115200);
    while(!Serial);

    UNITY_BEGIN();
    test_storage::run_tests();
    test_sensor::run_tests();
    test_display::run_tests();
    test_rtc::run_tets();
    test_hiking_station::run_tests();
    UNITY_END();
}


void loop(){}