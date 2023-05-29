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

void SPI::transfer_2(const unsigned char *tx, int tx_len, unsigned char *rx, int rx_len)
{
	if (tx_len == rx_len) {
                spi_write_read_blocking((spi_inst_t *)this->port, tx, rx, tx_len);
		debug("spi %d %02x %02x,%02x %02x\r\n",tx_len, tx[0], tx[1], rx[0], rx[1]);
        } else {
                debug("transfer_2 %d != %d\r\n",tx_len, rx_len);
        }
}

void Timeout::attach_us (void (*func)(void), us_timestamp_t t)
{
        debug("attach_us\r\n");
}

int Timer::read_us(void)
{
	return time_us_32();
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
	gpio_set_dir(pin, GPIO_OUT);
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
	if (this->pin != 9)
		debug("DigitalOut::write %d=%d\r\n",this->pin,value);
	gpio_put(this->pin, value);
}

InterruptIn::InterruptIn(int pin)
{
	this->pin=pin;
}

void InterruptIn::rise(void (*func)(void))
{
	debug("InterruptIn::rise\r\n");
}

unsigned short AnalogIn::read_u16(void)
{
	return 126;
}
