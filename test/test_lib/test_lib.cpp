#include "Arduino.h"
#include <unity.h>
#include "some_lib.h"

#include <test_runner.h>

void test_some_function(){
    print_hello_world();
    TEST_ASSERT_TRUE(true);
}

void setup(){
    Serial.begin(115200);
    while (!Serial){;}

    UNITY_BEGIN();
    RUN_TEST(test_some_function);
    UNITY_END();
}

void loop(){
}