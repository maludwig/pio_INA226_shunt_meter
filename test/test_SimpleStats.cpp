#include "Arduino.h"
#include "unity.h"
#include "SimpleStats.h"

void setUp(void) {
  // No setup required for this example
}

void tearDown(void) {
  // No teardown required for this example
}

void test_simple_stats(void) {
  SimpleStats stats;
  stats.add_measurement(10);
  stats.add_measurement(20);
  stats.add_measurement(30);
  TEST_ASSERT_EQUAL_INT32(20, stats.get_mean());
  TEST_ASSERT_EQUAL_INT32(10, stats.min);
  TEST_ASSERT_EQUAL_INT32(30, stats.max);
  TEST_ASSERT_EQUAL_UINT32(3, stats.count);
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_simple_stats);
  return UNITY_END();
}

// WARNING!!! PLEASE REMOVE UNNECESSARY MAIN IMPLEMENTATIONS //

/**
  * For native dev-platform or for some embedded frameworks
  */
int main(void) {
  return runUnityTests();
}

/**
  * For Arduino framework
  */
void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  delay(2000);

  runUnityTests();
}
void loop() {}
