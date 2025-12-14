#pragma once
#include <unity.h>
#include "display_manager.h"
#include "data_structures.h"
#include <logger.h>

DisplayManager testDisplay;

void test_display_initialization(){
    testDisplay.begin(TFT_CS, TFT_DC, TFT_RST, TFT_LIT);
    bool result = testDisplay.isReady();
    TEST_ASSERT_TRUE(result);
}

namespace test_display {
    void run_tests(){
        UnitySetTestFile(__FILE__);

        LOG_INFO("\n[RUN TEST] DISPLAY:\n");

        RUN_TEST(test_display_initialization);
    }
}
