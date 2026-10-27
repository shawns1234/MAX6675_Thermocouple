/**
  Host tests for SmoothThermocouple and MAX6675_Thermocouple.

  Build and run from the repository root:
    g++ -std=c++11 -O0 -Wall -Wextra -DARDUINO=100 \
      -Itest/host -Isrc test/host/test_thermocouple.cpp \
      src/SmoothThermocouple.cpp src/MAX6675_Thermocouple.cpp \
      -o /tmp/test_thermocouple
    /tmp/test_thermocouple

  -O0 matches a debug build, where an odr-used static const member
  without a definition fails to link.
*/
#include <stdio.h>

#include "Arduino.h"
#include "MAX6675_Thermocouple.h"
#include "SmoothThermocouple.h"

namespace {

int failures = 0;

// Model of the MAX6675 serial interface. CS low starts a frame with
// D15 on SO, and each SCK rising edge moves to the next bit, so a
// read that samples SO while SCK is low gets D15 first.
// The model also checks that every pin change and every SO sample is
// preceded by a delay since the previous pin change. Any delay of
// 1 us or more meets the 100 ns MAX6675 timing minimums.
const int SCK_PIN = 1;
const int CS_PIN = 2;
const int SO_PIN = 3;

struct Max6675Model {
  uint16_t frame = 0;
  int bit = 15;
  int cs = HIGH;
  int sck = LOW;
  bool delayedSinceChange = true;
  int timingViolations = 0;
  unsigned long totalMillis = 0;
  unsigned long totalMicros = 0;
} chip;

void checkDelay() {
  if (!chip.delayedSinceChange) {
    ++chip.timingViolations;
  }
}

}  // namespace

void pinMode(int, int) {}

void digitalWrite(int pin, int value) {
  if (pin == CS_PIN && value != chip.cs) {
    checkDelay();
    chip.cs = value;
    chip.bit = 15;
    chip.delayedSinceChange = false;
  } else if (pin == SCK_PIN && value != chip.sck) {
    // Only edges during a frame are timed; the bus is idle otherwise.
    if (chip.cs == LOW) {
      checkDelay();
      if (value == HIGH) {
        --chip.bit;
      }
      chip.delayedSinceChange = false;
    }
    chip.sck = value;
  }
}

int digitalRead(int pin) {
  if (pin != SO_PIN || chip.cs == HIGH || chip.bit < 0) {
    return LOW;
  }
  checkDelay();
  return (chip.frame >> chip.bit) & 1;
}

void delay(unsigned long ms) {
  chip.totalMillis += ms;
  chip.delayedSinceChange = true;
}

void delayMicroseconds(unsigned int us) {
  chip.totalMicros += us;
  chip.delayedSinceChange = true;
}

namespace {

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

// Reads one frame through the driver. Returns the temperature and
// leaves the timing counters in chip.
double readFrame(MAX6675_Thermocouple& t, uint16_t frame) {
  chip.frame = frame;
  chip.timingViolations = 0;
  chip.totalMillis = 0;
  chip.totalMicros = 0;
  // Reads are far apart in practice (one conversion takes up to 220 ms).
  chip.delayedSinceChange = true;
  return t.readCelsius();
}

void testMax6675Decode() {
  MAX6675_Thermocouple t(SCK_PIN, CS_PIN, SO_PIN);
  // D14..D3 hold the temperature in 0.25 C steps; D2 is the open
  // thermocouple flag.
  const uint16_t frames[] = {0x0000, 0x0008, 0x1900, 0x5550, 0x2AA8, 0x7FF8};
  for (uint16_t frame : frames) {
    char name[32];
    snprintf(name, sizeof(name), "decode 0x%04X", frame);
    expectNear(name, readFrame(t, frame), (frame >> 3) * 0.25);
    if (chip.timingViolations != 0) {
      printf("FAIL %s: %d timing violation(s)\n", name, chip.timingViolations);
      ++failures;
    }
  }
  expectNan("open thermocouple", readFrame(t, 0x1904));
}

void testMax6675ReadTime() {
  MAX6675_Thermocouple t(SCK_PIN, CS_PIN, SO_PIN);
  readFrame(t, 0x1900);
  // One setup delay after CS falls, then two per bit.
  if (chip.totalMillis != 0 || chip.totalMicros != 33) {
    printf("FAIL read time: expected 0 ms + 33 us of delay, got %lu ms + %lu us\n",
      chip.totalMillis, chip.totalMicros);
    ++failures;
  }
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
  testMax6675Decode();
  testMax6675ReadTime();
  if (failures == 0) {
    printf("OK: all tests passed\n");
    return 0;
  }
  printf("%d failure(s)\n", failures);
  return 1;
}
