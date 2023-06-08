#ifndef __MBED_H__
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "npr70piconfig.h"
extern "C" {
#include "npr70piextra.h"
}

#define INT_SRAM_SIZE (128*1024)
extern unsigned char int_sram[INT_SRAM_SIZE];

#define SERIAL_TX 0
#define SERIAL_RX 0
#define LWIP_ERR(x) (x-30)
#define LFS_ERR(x) (x)
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
typedef unsigned int ns_timestamp_t;

class SPI {
  void * port;
public:
  SPI(void * port);
  SPI(void * port, int miso, int clk, int mosi);
  void transfer_2(const unsigned char *tx, int tx_len, unsigned char *rx, int rx_len);
  void format(int f1, int f2);
  void frequency(int freq);
};

class Timeout {
   alarm_id_t alarm;
   bool active;
   void (*func)(void);
public:
   Timeout(void);
   void attach_us (void (*func)(void), us_timestamp_t t);
   void trigger(void);
};

class Timer {
   us_timestamp_t base;
public:
   Timer(void);
   void reset(void);
   void start(void);
   int read_us(void);
};

class AnalogIn {
   int pin;
public:
   AnalogIn(int pin);
   unsigned short read_u16();
};

class Gpio {
public:
  int pin;
  int state;
  Gpio(int pin);
  int read(void);
  void setstate(int state);
  operator int();
};

class DigitalIn : public Gpio {
public:
  DigitalIn(int pin) : Gpio(pin) {};
};

class DigitalOut : public Gpio {
public:
  DigitalOut(int pin);
  void write(int value);
  DigitalOut &operator= (int value) { write(value); return *this; }
};

class DigitalInOut : public DigitalOut {
public:
  DigitalInOut(int pin);
  void output(void);
  void input(void);
};

// typedef int DigitalOut;
class InterruptIn : public Gpio {
  int event;
  void (*func)(void);
public:
  InterruptIn(int pin);
  void rise(void (*func)(void));
  void fall(void (*func)(void));
  void trigger(void);
};

void NVIC_SystemReset(void);
void wait_ms(int ms);
void wait_us(us_timestamp_t us);
void wait_ns(us_timestamp_t ns);
#define __MBED_H__
#endif
