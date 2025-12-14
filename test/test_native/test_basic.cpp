#include <unity.h>

// Unity requires these functions
void setUp(void) {}

void tearDown(void) {}

// Very simple test - just math
void test_basic_math() {
    int result = 2 + 2;
    TEST_ASSERT_EQUAL(4, result);
}

// Test string comparison
void test_string_comparison() {
    const char* expected = "hello";
    const char* actual = "hello";
    TEST_ASSERT_EQUAL_STRING(expected, actual);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_math);
    RUN_TEST(test_string_comparison);
    UNITY_END();
    return 0;
}
