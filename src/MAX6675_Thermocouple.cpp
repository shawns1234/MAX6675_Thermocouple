/**
	Created by Yurii Salimov, February, 2018.
	Released into the public domain.
*/
#include "MAX6675_Thermocouple.h"

MAX6675_Thermocouple::MAX6675_Thermocouple(
	const int SCK_pin,
	const int CS_pin,
	const int SO_pin
) {
	pinMode(this->SCK_pin = SCK_pin, OUTPUT);
	pinMode(this->CS_pin = CS_pin, OUTPUT);
	pinMode(this->SO_pin = SO_pin, INPUT);
	digitalWrite(this->CS_pin, HIGH);
}

/**
	Serial timing, from the MAX6675 datasheet (19-2235, Rev 1):
	- fSCL, serial clock frequency: 4.3 MHz max;
	- tCH, tCL, SCK high and low widths: 100 ns min;
	- tCSS, CS fall to SCK rise: 100 ns min;
	- tDV, CS fall to output enable: 100 ns max;
	- tDO, SCK fall to output data valid: 100 ns max.
	Each wait below is 1 us, at least 10 times these figures. SCK runs
	at 500 kHz or less, and the waits add up to 33 us per read, where
	the 1 ms waits used before added up to 33 ms.

	Bringing CS low stops any conversion in progress, and bringing it
	high starts a new one. A conversion takes 0.17 s typical, 0.22 s max,
	so reads should be at least 220 ms apart to get a new value. A
	faster read does not change this.
*/
double MAX6675_Thermocouple::readCelsius() {
	digitalWrite(this->CS_pin, LOW);
	delayMicroseconds(1); // tCSS and tDV
	int value = spiread();
	value <<= 8;
	value |= spiread();
	digitalWrite(this->CS_pin, HIGH);
	if (value & 0x4) {
		return NAN;
	}
	value >>= 3;
	return (value * 0.25);
}

/**
	Returns a temperature in Kelvin.
	Reads the temperature in Celsius,
	converts in Kelvin and return it.

	@return temperature in degree Kelvin
*/
double MAX6675_Thermocouple::readKelvin() {
	return celsiusToKelvins(readCelsius());
}

/**
	Returns a temperature in Fahrenheit.
	Reads a temperature in Celsius,
	converts in Fahrenheit and return it.

	@return temperature in degree Fahrenheit
*/
double MAX6675_Thermocouple::readFahrenheit() {
	return celsiusToFahrenheit(readCelsius());
}

byte MAX6675_Thermocouple::spiread() {
	byte value = 0;
	for (int i = 7; i >= 0; --i) {
		digitalWrite(this->SCK_pin, LOW);
		delayMicroseconds(1); // tCL and tDO
		if (digitalRead(this->SO_pin)) {
			value |= (1 << i);
		}
		digitalWrite(this->SCK_pin, HIGH);
		delayMicroseconds(1); // tCH
	}
	return value;
}

inline double MAX6675_Thermocouple::celsiusToKelvins(const double celsius) {
	return (celsius + 273.15);
}

inline double MAX6675_Thermocouple::celsiusToFahrenheit(const double celsius) {
	return (celsius * 1.8 + 32);
}
