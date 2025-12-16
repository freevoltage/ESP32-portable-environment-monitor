#undef MOCK
#include <unity.h>
#include <test_runner.h>
#include <test_fixture.h>
#include "test_data_service.hpp"

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
    test_data_service::run_tests();
    UNITY_END();
}


void loop(){}