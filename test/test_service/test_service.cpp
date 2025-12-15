#undef MOCK
#include <unity.h>
#include <test_runner.h>
#include "test_data_service.hpp"


void setup(){
    Serial.begin(115200);
    while(!Serial);

    UNITY_BEGIN();
    test_data_service::run_tests();
    UNITY_END();
}


void loop(){}