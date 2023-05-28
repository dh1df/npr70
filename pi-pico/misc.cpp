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

void SPI::transfer_2(const unsigned char *tx, int tx_len, unsigned char *rx, int rx_len)
{
	if (tx_len == rx_len) {
                spi_write_read_blocking(SI4463_PORT_SPI, tx, rx, tx_len);
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

void DigitalInOut::write(int value)
{
	debug("DigitalInOut::write %d\r\n",value);
}

void DigitalOut::write(int value)
{
	debug("DigitalOut::write %d\r\n",value);
}


