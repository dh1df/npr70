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


#define LED_PIN     25

err_t error;

struct tcp_pcb *testpcb;

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

	gpio_put(LED_PIN, 1);

	/* create the control block */
	testpcb = tcp_new();	//testpcb is a global struct tcp_pcb
	// as defined by lwIP



	err = tcp_bind(testpcb, IP_ANY_TYPE, 23);
	if (err == ERR_OK) {
		testpcb = tcp_listen(testpcb);
		int s = TELNET_SOCKET;
		tcp_arg(testpcb, &W5500_channel[s]);
		tcp_accept(testpcb, W5500_accept);
	} else {
		/* abort? output diagnostic? */
	}


}

void
init_spi(void)
{
	spi_init(SPI_PORT, 1 * 1000 * 1000);
	gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
	gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
	gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
	gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

	// Chip select is active-low, so we'll initialise it to a driven-high state
	gpio_set_dir(PIN_CS, GPIO_OUT);
	gpio_put(PIN_CS, 1);
}

int
init_wifi(void)
{
	debug("cyw43_arch_init()\r\n");
	if (cyw43_arch_init()) {
		printf("failed to initialise\n");
		return 1;
	}
	debug("cyw43_arch_enable_sta_mode()\r\n");
	cyw43_arch_enable_sta_mode();
	// this seems to be the best be can do using the predefined `cyw43_pm_value` macro:
	// cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);
	// however it doesn't use the `CYW43_NO_POWERSAVE_MODE` value, so we do this instead:
	cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 20, 1, 1, 1));

	debug("Connecting to WiFi...\r\n");
	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
		debug("failed to connect.\r\n");
		return 1;
	} else {
		debug("Connected.\r\n");

		extern cyw43_t cyw43_state;
		auto ip_addr = cyw43_state.netif[CYW43_ITF_STA].ip_addr.addr;
		debug("IP Address: %lu.%lu.%lu.%lu\r\n", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24);
		return 0;
	}
}

extern "C" void enchw_init(void);
extern "C" void test(void);

void
test(void)
{
#if 0
	debug("enchw_init()\r\n");
	enchw_init();
#else
	debug("init_wifi()\r\n");
	init_wifi();
#endif
}

int main()
{
#if 0
	stdio_uart_init();
#else
	stdio_uart_init_full(uart_default, 460800, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
#endif
	debug("init_spi()\r\n");
	init_spi();
	

	// Initialize tinyusb, lwip, dhcpd and httpd
	init_lwip();
	wait_for_netif_is_up();
	dhcpd_init();

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	gpio_put(LED_PIN, 1);
	tcp_setup();

	while (true) {
		tud_task();
		service_traffic();
		misc_loop();
	}

	return 0;
}
