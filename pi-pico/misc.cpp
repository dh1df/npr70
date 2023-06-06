#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "../source/HMI_telnet.h"
#include "../source/DHCP_ARP.h"
#include "mbed.h"

unsigned char int_sram[INT_SRAM_SIZE];

void ext_SRAM_read2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size)
{
	memcpy(loc_data, int_sram+address, size);
}
void ext_SRAM_write2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size)
{
	memcpy(int_sram+address, loc_data, size);
}

void NVIC_SystemReset(void)
{ 
	watchdog_reboot(0, SRAM_END, 1);
}

void wait_ms(int ms)
{
	busy_wait_us(ms*1000LL);
}

void wait_us(us_timestamp_t us)
{
	busy_wait_us(us);
}

void wait_ns(ns_timestamp_t ns)
{
	// cycles = ns * clk_sys_hz / 1,000,000,000
	uint32_t cycles = ns * (clock_get_hz(clk_sys) >> 16u) / (1000000000u >> 16u);
	busy_wait_at_least_cycles(cycles);
}

void call_bootloader(void)
{
	reset_usb_boot(0,0);
}

SPI::SPI(void *port)
{
	this->port=port;
}

SPI::SPI(void *port, int miso, int clk, int mosi)
{
	this->port=port;
	spi_init((spi_inst_t *)port, 1 * 1000 * 1000);
        gpio_set_function(miso, GPIO_FUNC_SPI);
        gpio_set_function(clk, GPIO_FUNC_SPI);
        gpio_set_function(mosi, GPIO_FUNC_SPI);

}

void SPI::transfer_2(const unsigned char *tx, int tx_len, unsigned char *rx, int rx_len)
{
	if (tx_len == rx_len) {
                spi_write_read_blocking((spi_inst_t *)this->port, tx, rx, tx_len);
		// debug("spi %d %02x %02x,%02x %02x\r\n",tx_len, tx[0], tx[1], rx[0], rx[1]);
        } else {
                debug("transfer_2 %d != %d\r\n",tx_len, rx_len);
        }
}

void SPI::frequency(int freq)
{
}

void SPI::format(int f1, int f2)
{
}

static int64_t timeout_callback(alarm_id_t id, void *user_data)
{
	// debug("timeout_callback %d %p\r\n",id,user_data);
	Timeout *timeout=(Timeout *)user_data;
	timeout->trigger();
	return 0;
}

Timeout::Timeout(void)
{
	active=false;
}

void Timeout::attach_us (void (*func)(void), us_timestamp_t t)
{
        // debug("attach_us %d %p %p\r\n",t,&timer,this);
	if (active)
		cancel_alarm(alarm);
	this->func=func;
	alarm=add_alarm_in_us(t, timeout_callback, this, true);
	active=true;
}

void Timeout::trigger(void)
{
	active=false;
	func();
}

Timer::Timer(void)
{
	base=0;
}

int Timer::read_us(void)
{
	return time_us_32()-base;
}

void Timer::reset(void)
{
	base=time_us_32();
}

void Timer::start(void)
{
	reset();
}

DigitalInOut::DigitalInOut(int pin) : DigitalOut(pin)
{
	gpio_set_dir(pin, GPIO_IN);
	gpio_set_pulls(pin, true, false);
}

DigitalOut::DigitalOut(int pin) : Gpio(pin)
{
	if (pin != -1) {
		gpio_set_function(pin, GPIO_FUNC_SIO);
		gpio_set_dir(pin, GPIO_OUT);
	}
}

Gpio::Gpio(int pin)
{
	this->pin=pin;
}

int Gpio::read()
{
	if (this->pin == -1)
		return this->state;
	return gpio_get(this->pin);
}

void Gpio::setstate(int state)
{
	this->state=state;
}

Gpio::operator int()
{
	return this->read();
}

void DigitalInOut::output(void)
{
	gpio_set_dir(pin, GPIO_OUT);
}

void DigitalInOut::input(void)
{
	gpio_set_dir(pin, GPIO_IN);
}


void DigitalOut::write(int value)
{
	// debug("DigitalOut::write %d=%d\r\n",this->pin,value);
	if (this->pin != -1)
		gpio_put(this->pin, value);
}

extern InterruptIn Int_SI4463;

static void irq_callback(uint gpio, uint32_t event_mask)
{
	Int_SI4463.trigger();
}


InterruptIn::InterruptIn(int pin) : Gpio(pin)
{
	this->pin=pin;
	this->event=0;
}

void InterruptIn::rise(void (*func)(void))
{
	debug("InterruptIn::rise\r\n");
}

void InterruptIn::fall(void (*func)(void))
{
	// debug("InterruptIn::fall %d\r\n",this->read());
#if 0
	this->event=GPIO_IRQ_LEVEL_LOW;
#else
	this->event=GPIO_IRQ_EDGE_FALL;
#endif
	this->func=func;
	gpio_set_irq_enabled_with_callback(this->pin, this->event, true, irq_callback);
}

void InterruptIn::trigger()
{
	this->func();
}

AnalogIn::AnalogIn(int pin)
{
	this->pin=pin;
	adc_init();
	adc_gpio_init(26);
	adc_select_input(0);

}

unsigned short AnalogIn::read_u16(void)
{
	return adc_read() << 4;
}
