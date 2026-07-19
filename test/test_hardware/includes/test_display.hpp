#pragma once
#include <unity.h>
#include "display_manager.h"
#include "data_structures.h"
#include <logger.h>
#include <test_fixture.h>

DisplayManager testDisplay;

void test_display_initialization(){
    testDisplay.begin();
    bool result = testDisplay.isReady();
    TEST_ASSERT_TRUE(result);
}

namespace test_display {

    void setUp(){}

    void tearDown(){}

    void run_tests(){
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        LOG_INFO("\n[RUN TEST] DISPLAY:\n");

        RUN_TEST(test_display_initialization);

        TEST_CONTEXT.clearFixtures();
    }
}
