#include <stdarg.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "tusb_lwip_glue.h"

#include "lwip/tcp.h"
#include "W5500_lwip.h"
#include "npr70piconfig.h"
#include "npr70.h"
#include "../source/config_flash.h"
#include "../source/SI4463.h"
#include "../source/TDMA.h"
#include "../source/L1L2_radio.h"
#include "../source/DHCP_ARP.h"
#include "../source/signaling.h"
#include "../source/Eth_IPv4.h"
#include "common.h"
#include "main.h"

#define WATCHDOG_TIMEOUT 0x7fffff

err_t error;

SPI spi_0(spi0, SPI0_PIN_MISO, SPI0_PIN_SCK, SPI0_PIN_MOSI);
SPI spi_1(spi1, SPI1_PIN_MISO, SPI1_PIN_SCK, SPI1_PIN_MOSI);
SPI spi_2(NULL);
DigitalOut CS1(-1);
DigitalOut CS2(SI4463_PIN_CS);
InterruptIn Int_SI4463(SI4463_PIN_INT);
DigitalIn Int_W5500(-1);
DigitalOut CS4(ENC_PIN_CS);

AnalogIn Random_pin(RANDOM_PIN);
DigitalOut LED_RX_loc(LED_RX_PIN);
DigitalOut LED_connected(LED_CONN_PIN);
DigitalOut SI4463_SDN(SI4463_PIN_SDN);
DigitalInOut FDD_trig_pin(GPIO_11_PIN);
InterruptIn FDD_trig_IRQ(GPIO_11_PIN);
DigitalOut PTT_PA_pin(GPIO_10_PIN);

void debug(const char *str, ...)
{
	va_list ap;
	va_start(ap, str);
#if 0
	char buffer[128];
	vsnprintf(buffer, sizeof(buffer), str, ap);
	W5500_transmit(&W5500_channel[TELNET_SOCKET], (unsigned char *)buffer, strlen(buffer));
#else
	vprintf(str, ap);
#endif
	va_end(ap);
}

enum retcode
cmd_test(struct context *ctx)
{
	HMI_cprintf(ctx, "%d\r\n", Int_SI4463.read());
	return RET_OK_PROMPT;
}

int main()
{
	stdio_uart_init_full(uart_default, 921600, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
	gpio_set_function(ENC_PIN_INT, GPIO_FUNC_SIO);
	gpio_set_dir(ENC_PIN_INT, GPIO_IN);
	CS4=1;

	wait_ms(200);
	debug("\r\n\r\nNPR FW %s\r\n", FW_VERSION);

	littlefs_init();
	NFPR_config_read(&Random_pin);

	init1();
	init2();
	HMI_prompt(NULL);

	watchdog_enable(WATCHDOG_TIMEOUT, true);
	while (true) {
		loop();
		trace_loop();
		tud_task();
		service_traffic();
		cyw43_arch_poll();
		enchw_poll();
		watchdog_update();
	}

	return 0;
}
