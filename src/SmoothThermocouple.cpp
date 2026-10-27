/**
  Created by Yurii Salimov, May, 2019.
  Released into the public domain.
*/
#include "SmoothThermocouple.h"

SmoothThermocouple::SmoothThermocouple(
  Thermocouple* origin,
  const int smoothingFactor
) {
  this->origin = origin;
  setSmoothingFactor(smoothingFactor);
}

SmoothThermocouple::~SmoothThermocouple() {
  delete this->origin;
}

double SmoothThermocouple::readCelsius() {
  return smoothe(
    this->origin->readCelsius(),
    this->celsius, this->hasCelsius
  );
}

double SmoothThermocouple::readKelvin() {
  return smoothe(
    this->origin->readKelvin(),
    this->kelvin, this->hasKelvin
  );
}

double SmoothThermocouple::readFahrenheit() {
  return smoothe(
    this->origin->readFahrenheit(),
    this->fahrenheit, this->hasFahrenheit
  );
}

double SmoothThermocouple::smoothe(
  const double input, double& data, bool& hasData
) {
  if (isnan(input)) {
    hasData = false;
    return input;
  }
  data = hasData
    ? ((data * (this->smoothingFactor - 1) + input) / this->smoothingFactor)
    : input;
  hasData = true;
  return data;
}

/**
  Compares by value instead of calling max(). Some cores implement
  max() with const reference parameters, which odr-uses
  MIN_SMOOTHING_FACTOR. With no out-of-line definition of the constant,
  that fails to link in unoptimised (-O0) builds.
*/
inline void SmoothThermocouple::setSmoothingFactor(const int smoothingFactor) {
  this->smoothingFactor = (smoothingFactor < MIN_SMOOTHING_FACTOR)
    ? MIN_SMOOTHING_FACTOR
    : smoothingFactor;
}
