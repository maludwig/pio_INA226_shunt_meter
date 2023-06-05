#include <unity.h>
#include "functions.h"  // Assuming the si_format function is in this file

void test_si_format() {
    char buffer[20];

    si_format(1.0, 5, 1, buffer);
    TEST_ASSERT_EQUAL_STRING("  1.0", buffer);

    si_format(1.12, 5, 1, buffer);
    TEST_ASSERT_EQUAL_STRING("  1.1", buffer);

    si_format(-1.12, 5, 1, buffer);
    TEST_ASSERT_EQUAL_STRING(" -1.1", buffer);

    si_format(-1.123, 5, 2, buffer);
    TEST_ASSERT_EQUAL_STRING("-1.12", buffer);

    si_format(-0.0123, 5, 0, buffer);
    TEST_ASSERT_EQUAL_STRING(" -12m", buffer);

    si_format(0.0123467, 5, 1, buffer);
    TEST_ASSERT_EQUAL_STRING("12.3m", buffer);

    si_format(0.0123467, 5, 2, buffer);
    TEST_ASSERT_EQUAL_STRING("12.35m", buffer);

    si_format(0.0123467, 5, 4, buffer);
    TEST_ASSERT_EQUAL_STRING("12.3467m", buffer);

    si_format(0.0000123467, 5, 2, buffer);
    TEST_ASSERT_EQUAL_STRING("12.35u", buffer);
    
    si_format(0.0000123467, 10, 2, buffer);
    TEST_ASSERT_EQUAL_STRING("    12.35u", buffer);
    
    si_format(0.0, 10, 2, buffer);
    TEST_ASSERT_EQUAL_STRING("      0.00", buffer);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_si_format);
    UNITY_END();
}

void loop() {
    // Do nothing here.
}
