#undef MOCK
#include <unity.h>
#include <test_runner.h>
#include "test_storage.hpp"
#include "test_sensor.hpp"
#include "test_display.hpp"
#include "test_rtc.hpp"

StorageManager storage;

void setup(){
    Serial.begin(115200);
    while(!Serial);
    (void) storage;

    UNITY_BEGIN();
    RUN_TEST(test_storage::run_tests);
    RUN_TEST(test_sensor::run_tests);
    RUN_TEST(test_display::run_tests);
    RUN_TEST(test_rtc::run_tets);
    UNITY_END();
}


void loop(){}