#include <stdarg.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "tusb_lwip_glue.h"

#include "lwip/tcp.h"
#include "W5500_lwip.h"


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

int main()
{
#if 0
	stdio_uart_init();
#else
	stdio_uart_init_full(uart_default, 460800, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
#endif

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
