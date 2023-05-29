#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/spi.h"
#include "../source/HMI_telnet.h"
#include "../source/DHCP_ARP.h"
#include "mbed.h"

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

void misc_loop(void)
{
	telnet_loop(NULL); 
	serial_term_loop();
	DHCP_server(&LAN_conf_applied, W5500_p1);
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

void Timeout::attach_us (void (*func)(void), us_timestamp_t t)
{
        debug("attach_us\r\n");
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

DigitalInOut::DigitalInOut(int pin)
{
	gpio_set_dir(pin, GPIO_IN);
	gpio_set_pulls(pin, true, false);
	this->pin=pin;
}

DigitalOut::DigitalOut(int pin)
{
	this->pin=pin;
	gpio_set_function(pin, GPIO_FUNC_SIO);
	gpio_set_dir(pin, GPIO_OUT);
}

int DigitalIn::read()
{
	return gpio_get(this->pin);
}

DigitalIn::operator int()
{
	return this->read();
}

void DigitalInOut::write(int value)
{
	debug("DigitalInOut::write %d=%d\r\n",this->pin,value);
	gpio_put(this->pin, value);
}

void DigitalInOut::output(void)
{
	gpio_set_dir(pin, GPIO_OUT);
}


void DigitalOut::write(int value)
{
	// debug("DigitalOut::write %d=%d\r\n",this->pin,value);
	gpio_put(this->pin, value);
}

extern InterruptIn Int_SI4463;

static void irq_callback(uint gpio, uint32_t event_mask)
{
	Int_SI4463.trigger();
}


InterruptIn::InterruptIn(int pin)
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
	debug("InterruptIn::fall\r\n",this->read());
	this->event=GPIO_IRQ_LEVEL_LOW;
	this->func=func;
	gpio_set_irq_enabled_with_callback(this->pin, this->event, true, irq_callback);
}

int InterruptIn::read()
{
	return gpio_get(this->pin);
}

void InterruptIn::trigger()
{
	this->func();
}

InterruptIn::operator int()
{
	return this->read();
}

unsigned short AnalogIn::read_u16(void)
{
	return 126;
}
