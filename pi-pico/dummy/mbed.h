#ifndef __MBED_H__
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#define SERIAL_TX 0
#define SERIAL_RX 0
class Serial {
public:
	Serial(int a, int b) {
	};
};

class SPI {
};
class Timeout {
};
class Timer {
public:
   int read_us(void) { return time_us_32(); }
};
typedef int AnalogIn;
typedef int DigitalIn;
typedef int DigitalInOut;
typedef int DigitalOut;
typedef int InterruptIn;
void NVIC_SystemReset(void);
void wait_ms(int ms);
extern "C" void misc_loop(void);
extern "C" void debug(const char *str, ...);
#define __MBED_H__
#endif
