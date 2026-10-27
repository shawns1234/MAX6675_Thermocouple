# MAX6675 Thermocouple Library

For Arduino boards.

The Library implements a set of methods for working with a thermocouple based on the MAX6675 driver.
Provides a temperature reading in Celsius, Fahrenheit and Kelvin.
Modified version of the [MAX6675-library](https://github.com/adafruit/MAX6675-library).

MAX6675 driver measures the output of a K Thermocouple and provides the result to the Arduino via a SPI interface.

## Maintained fork

This repository is a maintained fork of
[YuriiSalimov/MAX6675_Thermocouple](https://github.com/YuriiSalimov/MAX6675_Thermocouple),
which is no longer updated. The original author credit and the public
domain notice in the source files are unchanged.

### PlatformIO

```ini
lib_deps = https://github.com/shawns1234/MAX6675_Thermocouple.git#v2.0.3
```

Do not also list the registry package `yuriisalimov/MAX6675_Thermocouple`.
Both provide the same classes, so PlatformIO installs and builds two
copies and the link fails with multiple definition errors.

### Changes in 2.0.3

- `SmoothThermocouple` no longer stays at NAN after one NAN reading.
  Before, a single open-thermocouple reading was blended into the
  average, and every later reading returned NAN until reboot. Now the
  NAN is returned for that read, and the next valid reading starts a new
  average. Starting fresh, rather than blending with the value from
  before the fault, avoids a false slow ramp when a probe is reconnected
  at a different temperature.
- `SmoothThermocouple` no longer restarts the average when the smoothed
  value is exactly 0. A seeded flag replaces the old zero check.
- `SmoothThermocouple` links in unoptimised (-O0) builds. Before, cores
  whose `max()` takes const references failed with an undefined
  reference to `SmoothThermocouple::MIN_SMOOTHING_FACTOR`. Projects that
  added their own definition of the constant as a workaround still link.
- `MAX6675_Thermocouple` reads in about 33 us instead of about 33 ms. The
  1 ms waits on each clock edge are now 1 us, which is still at least 10
  times the 100 ns timing figures in the MAX6675 datasheet. The bit sequence
  and decoded values are unchanged.
- Host tests in `test/host`.

The MAX6675 still needs up to 220 ms per conversion, and each read
restarts the conversion, so keep reads at least 220 ms apart to get a
new value.

### Tests

The host tests build with g++ against a minimal Arduino stub. From the
repository root:

```sh
g++ -std=c++11 -O0 -Wall -Wextra -DARDUINO=100 \
  -Itest/host -Isrc test/host/test_thermocouple.cpp \
  src/SmoothThermocouple.cpp src/MAX6675_Thermocouple.cpp \
  -o /tmp/test_thermocouple
/tmp/test_thermocouple
```

## Installation

1. [Download](https://github.com/shawns1234/MAX6675_Thermocouple/tags) the latest release from GitHub.
2. Unzip and modify the Folder name to "MAX6675_Thermocouple" (Remove the '-version')
3. Paste the modified folder on your Library folder (On your `libraries` folder inside Sketchbooks or Arduino software).
4. Restart the Arduino IDE.

## Circuit Diagram

![Circuit Diagram](CircuitDiagram.png)

SO: The module’s serial output. Your Arduino will read this output.

CS: Chip Select. Setting low, selects the Module and tells it to supply an output that is synchronize with a clock.

SCK: The Serial Clock, an input from your Arduino.

VCC: 5V supply

GND: Ground.

## Methods

```cpp
    // Instantiation:
    /**
        SCK_PIN - SCK digital port number.
        CS_PIN - CS digital port number.
        SO_PIN - SO digital port number.
    */
    MAX6675_Thermocouple thermocouple(SCK_PIN, CS_PIN, SO_PIN);

    // Read a temperature in Celsius.
    double celsius = thermocouple.readCelsius();
    // Read a temperature in Kelvin.
    double Kelvin = thermocouple.readKelvin();
    // Read a temperature in Fahrenheit.
    double fahrenheit = thermocouple.readFahrenheit();
```

### Examples

[Serial Reading](/examples/SerialReading/SerialReading.ino)

[Average Measurement](/examples/AverageMeasurement/AverageMeasurement.ino)

[Smooth Measurement](/examples/SmoothMeasurement/SmoothMeasurement.ino)

[All examples...](/examples)

Created by Yurii Salimov.
