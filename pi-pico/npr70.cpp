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


err_t error;

SPI spi_0(spi0, SPI0_PIN_MISO, SPI0_PIN_SCK, SPI0_PIN_MOSI);
SPI spi_1(spi1, SPI1_PIN_MISO, SPI1_PIN_SCK, SPI1_PIN_MOSI);
DigitalOut CS2(SI4463_PIN_CS);
InterruptIn Int_SI4463(SI4463_PIN_INT);

AnalogIn Random_pin(RANDOM_PIN);
DigitalOut LED_RX_loc(LED_RX_PIN);
DigitalOut LED_connected(LED_CONN_PIN);
DigitalOut SI4463_SDN(SI4463_PIN_SDN);
DigitalInOut FDD_trig_pin(GPIO_11_PIN);
InterruptIn FDD_trig_IRQ(GPIO_11_PIN);
DigitalOut PTT_PA_pin(GPIO_10_PIN);

static unsigned int timer_snapshot;
static Timer slow_timer;
static int temperature_timer;
static int slow_action_counter;
static int signaling_counter;


void debug(const char *str, ...)
{
	va_list ap;
	char buffer[128];
	va_start(ap, str);
#if 0
	vsnprintf(buffer, sizeof(buffer), str, ap);
	W5500_transmit(&W5500_channel[TELNET_SOCKET], (unsigned char *)buffer, strlen(buffer));
#else
	vprintf(str, ap);
#endif
	va_end(ap);
}


void tcp_setup(void)
{
	err_t err;
	struct W5500_channel *c;
	gpio_put(LED_PIN, 1);

	c=W5500_chan(TELNET_SOCKET);
	err=c?ERR_OK:ERR_ARG;
	if (err == ERR_OK) {
		c->tcp=tcp_new();
		if (!c->tcp)
			err=ERR_MEM;
	}
	if (err == ERR_OK)
		err = tcp_bind(c->tcp, IP_ANY_TYPE, 23);
	if (err == ERR_OK) {
		c->tcp=tcp_listen(c->tcp);
		if (!c->tcp)
			err=ERR_MEM;
	}
	if (err == ERR_OK) {
		tcp_arg(c->tcp, c);
		tcp_accept(c->tcp, W5500_accept);
	}
	debug("telnet %d\r\n",err);
}

void
init_spi(void)
{
	gpio_set_function(ENC_PIN_CS, GPIO_FUNC_SIO);
	gpio_set_dir(ENC_PIN_CS, GPIO_OUT);
	gpio_put(ENC_PIN_CS, 1);

}

int
init_wifi(void)
{
	debug("cyw43_arch_init()\r\n");
	debug("cyw43_arch_enable_sta_mode()\r\n");
	cyw43_arch_enable_sta_mode();
	// this seems to be the best be can do using the predefined `cyw43_pm_value` macro:
	// cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);
	// however it doesn't use the `CYW43_NO_POWERSAVE_MODE` value, so we do this instead:
	cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 20, 1, 1, 1));

	if (cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) {
		debug("failed wifi connect\r\n");
	} else {
		debug("wifi connecting\r\n");
	}
	return 0;
}

extern "C" void enchw_init(void);
extern "C" void test(void);

int
cmd_test(struct context *ctx)
{
#if 0
	if (s1) {
		if (!strcmp(s1,"1")) {
			debug("enchw_init()\r\n");
			enchw_init();
		}
		if (!strcmp(s1,"2")) {
			debug("init_wifi()\r\n");
			init_wifi();
		}
		if (!strcmp(s1,"3")) {
			debug("bridge_setup()\r\n");
			bridge_setup();
		}
	}
#endif
#if 0
	config_test();
	config_read((char *)int_sram, INT_SRAM_SIZE);
#endif
	return 3;
}

int main()
{
	int i;
	int wifi=0;
	stdio_uart_init_full(uart_default, 460800, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
	wait_ms(200);
	debug("\r\n\r\nNPR FW %s\r\n", FW_VERSION);

	littlefs_init();
	NFPR_config_read(&Random_pin);

	debug("init_spi()\r\n");
	init_spi();
	init1();
	init2();

#if 1
	// Initialize tinyusb, lwip, dhcpd and httpd
	if (cyw43_arch_init()) {
		wifi=0;
		printf("failed to initialise\n");
	}
#else
	lwip_init();
#endif
	tud_setup();
	enchw_init();
	if (wifi) 
		init_wifi();
	bridge_setup();

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	tcp_setup();
	gpio_put(LED_PIN, 1);

	while (true) {
		loop();
		tud_task();
		service_traffic();
		cyw43_arch_poll();
		enchw_poll();
	}

	return 0;
}
