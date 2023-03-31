#ifndef __MBED_H__
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

extern "C" void debug(const char *str, ...);
#define SERIAL_TX 0
#define SERIAL_RX 0
class Serial {
public:
	Serial(int a, int b) {
	};
	int readable(void) {
		return uart_is_readable(uart_default);
	}
	void sputc(int c) {
		putc(c,stdout);
	}
	int sgetc(void) {
		return getc(stdin);
	}
};

#define pico_getc(s) s.sgetc()
#define pico_putc(c,s) s.sputc(c)

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
#define __MBED_H__
#endif
