/**
  Host tests for SmoothThermocouple.

  Build and run from the repository root:
    g++ -std=c++11 -O0 -Wall -Wextra -DARDUINO=100 \
      -Itest/host -Isrc test/host/test_thermocouple.cpp \
      src/SmoothThermocouple.cpp -o /tmp/test_thermocouple
    /tmp/test_thermocouple

  -O0 matches a debug build, where an odr-used static const member
  without a definition fails to link.
*/
#include <stdio.h>

#include "Arduino.h"
#include "SmoothThermocouple.h"

void pinMode(int, int) {}
void digitalWrite(int, int) {}
int digitalRead(int) { return LOW; }
void delay(unsigned long) {}
void delayMicroseconds(unsigned int) {}

namespace {

int failures = 0;

// Returns a scripted sequence of readings. Each unit reads from its
// own copy of the script so the three smoothing states stay independent.
class ScriptedThermocouple final : public Thermocouple {
  public:
    ScriptedThermocouple(const double* values, int count)
      : values(values), count(count) {}

    double readCelsius() override { return next(c); }
    double readKelvin() override { return next(k); }
    double readFahrenheit() override { return next(f); }

  private:
    const double* values;
    int count;
    int c = 0;
    int k = 0;
    int f = 0;

    double next(int& index) {
      return (index < count) ? values[index++] : NAN;
    }
};

void expectNear(const char* name, double actual, double expected) {
  if (isnan(actual) || fabs(actual - expected) > 1e-9) {
    printf("FAIL %s: expected %.6f, got %.6f\n", name, expected, actual);
    ++failures;
  }
}

void expectNan(const char* name, double actual) {
  if (!isnan(actual)) {
    printf("FAIL %s: expected NAN, got %.6f\n", name, actual);
    ++failures;
  }
}

void testFirstSampleSeedsAverage() {
  const double values[] = {200.0, 100.0};
  SmoothThermocouple t(new ScriptedThermocouple(values, 2), 5);
  expectNear("seed", t.readCelsius(), 200.0);
  // (200 * 4 + 100) / 5
  expectNear("seed then smooth", t.readCelsius(), 180.0);
}

void testRecoversAfterNan() {
  const double values[] = {100.0, 100.0, NAN, 100.0, 150.0};
  SmoothThermocouple t(new ScriptedThermocouple(values, 5), 5);
  expectNear("before fault 1", t.readCelsius(), 100.0);
  expectNear("before fault 2", t.readCelsius(), 100.0);
  expectNan("fault", t.readCelsius());
  expectNear("after fault reseeds", t.readCelsius(), 100.0);
  // (100 * 4 + 150) / 5
  expectNear("after fault smooths", t.readCelsius(), 110.0);
}

void testNanReseedsWithNewValue() {
  // The value after a fault seeds a new average and is not blended
  // with the value from before the fault.
  const double values[] = {20.0, NAN, 180.0};
  SmoothThermocouple t(new ScriptedThermocouple(values, 3), 5);
  expectNear("before fault", t.readCelsius(), 20.0);
  expectNan("fault", t.readCelsius());
  expectNear("reseed", t.readCelsius(), 180.0);
}

void testZeroKeepsAverage() {
  const double values[] = {10.0, 0.0, 0.0};
  SmoothThermocouple t(new ScriptedThermocouple(values, 3), 5);
  expectNear("seed", t.readCelsius(), 10.0);
  // (10 * 4 + 0) / 5
  expectNear("zero 1", t.readCelsius(), 8.0);
  // (8 * 4 + 0) / 5
  expectNear("zero 2", t.readCelsius(), 6.4);
}

void testZeroSeed() {
  const double values[] = {0.0, 10.0};
  SmoothThermocouple t(new ScriptedThermocouple(values, 2), 5);
  expectNear("zero seed", t.readCelsius(), 0.0);
  // (0 * 4 + 10) / 5
  expectNear("after zero seed", t.readCelsius(), 2.0);
}

void testUnitsAreIndependent() {
  const double values[] = {100.0, NAN, 100.0};
  SmoothThermocouple t(new ScriptedThermocouple(values, 3), 5);
  expectNear("celsius seed", t.readCelsius(), 100.0);
  expectNear("kelvin seed", t.readKelvin(), 100.0);
  expectNan("celsius fault", t.readCelsius());
  // The Celsius fault does not clear the Kelvin or Fahrenheit state.
  expectNan("kelvin fault", t.readKelvin());
  expectNear("fahrenheit seed", t.readFahrenheit(), 100.0);
  expectNear("celsius reseed", t.readCelsius(), 100.0);
}

void testMinimumSmoothingFactor() {
  const double values[] = {100.0, 0.0};
  // A factor below the minimum of 2 is raised to 2.
  SmoothThermocouple t(new ScriptedThermocouple(values, 2), 0);
  expectNear("min factor seed", t.readCelsius(), 100.0);
  expectNear("min factor smooth", t.readCelsius(), 50.0);
}

}  // namespace

int main() {
  testFirstSampleSeedsAverage();
  testRecoversAfterNan();
  testNanReseedsWithNewValue();
  testZeroKeepsAverage();
  testZeroSeed();
  testUnitsAreIndependent();
  testMinimumSmoothingFactor();
  if (failures == 0) {
    printf("OK: all tests passed\n");
    return 0;
  }
  printf("%d failure(s)\n", failures);
  return 1;
}
