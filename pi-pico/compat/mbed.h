#ifndef __MBED_H__
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "npr70piconfig.h"

extern "C" void debug(const char *str, ...);
extern "C" void net_display(void);
extern "C" void cmd_test(char *s1, char *s2);
extern "C" unsigned int virt_EEPROM_write(void *data, unsigned int previous_index);
extern "C" void virt_EEPROM_errase_all(void);
extern "C" unsigned int virt_EEPROM_read(void *data);

#define HAVE_CALL_BOOTLOADER 1
#define HAVE_DISPLAY_NET 1
#define HAVE_CMD_TEST 1
#define HAVE_EXTERNAL_EEPROM_CONFIG 1
#define SKIP_UNIMPLEMENTED 1


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
		fflush(stdout);
	}
	int sgetc(void) {
		return getc(stdin);
	}
};

#define getc(s) s.sgetc()
#define putc(c,s) s.sputc(c)

typedef unsigned int us_timestamp_t;

class SPI {
public:
  void transfer_2(const unsigned char *tx, int tx_len, unsigned char *rx, int rx_len);
};

class Timeout {
public:
   void attach_us (void (*func)(void), us_timestamp_t t);
};
class Timer {
public:
   int read_us(void);
};
class AnalogIn {
};

class DigitalIn {
public:
  int read(void);
  operator int();
};

class DigitalOut {
public:
   void write(int value);
};

class DigitalInOut {
public:
   void write(int value);
};

// typedef int DigitalOut;
class InterruptIn {
public:
  int read(void);
  operator int();
};
void NVIC_SystemReset(void);
void wait_ms(int ms);
void wait_us(us_timestamp_t us);
extern "C" void misc_loop(void);
extern "C" void call_bootloader(void);
#define __MBED_H__
#endif
