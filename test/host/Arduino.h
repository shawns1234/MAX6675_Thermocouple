/**
  Minimal Arduino API stub for building the library on a host
  computer. Pin I/O is forwarded to hooks that the test defines,
  so a test can model the MAX6675 serial interface.
*/
#ifndef HOST_ARDUINO_STUB_H
#define HOST_ARDUINO_STUB_H

#include <math.h>
#include <stdint.h>

typedef uint8_t byte;

#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1

void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
int digitalRead(int pin);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

#endif
