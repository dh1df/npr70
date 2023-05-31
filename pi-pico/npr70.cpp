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
#include "../source/config_flash.h"
#include "../source/SI4463.h"
#include "../source/global_variables.h"
#include "../source/TDMA.h"
#include "../source/L1L2_radio.h"
#include "../source/HMI_telnet.h"
#include "../source/DHCP_ARP.h"
#include "../source/signaling.h"
#include "../source/Eth_IPv4.h"
#include "mbed.h"
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
#if 0

	c=W5500_chan(DHCP_SOCKET);
	err=c?ERR_OK:ERR_ARG;
	if (err == ERR_OK) {
		c->udp=udp_new();
		if (!c->udp)
			err=ERR_MEM;
	}
	if (err == ERR_OK)
		err=udp_bind(c->udp, IP_ANY_TYPE, 67);
	if (err == ERR_OK) 
		udp_recv(c->udp, W5500_udp_recv, c);
	debug("dhcp %d\r\n", err);
#endif
	

}

void
init_spi(void)
{
	gpio_set_function(ENC_PIN_CS, GPIO_FUNC_SIO);
	gpio_set_dir(ENC_PIN_CS, GPIO_OUT);
	gpio_put(ENC_PIN_CS, 1);

}

void
init_si4463(void)
{
	static SI4463_Chip SI4463_1;
        SI4463_1.spi = &spi_1;//1
        SI4463_1.cs = &CS2;//2
        SI4463_1.interrupt = &Int_SI4463;
        SI4463_1.RX_LED = &LED_RX_loc;
        SI4463_1.SDN = &SI4463_SDN;

        G_SI4463 = &SI4463_1;

        G_FDD_trig_pin = &FDD_trig_pin;
        G_FDD_trig_IRQ = &FDD_trig_IRQ;
        G_PTT_PA_pin = &PTT_PA_pin;
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
#if 1
	SI4463_SDN = 0;
	init_si4463();
	wait_ms(300);
#endif
        int i = SI4463_configure_all();
        if (i == 1) {
                debug("SI4463 configured\r\n");
        } else {
                debug("SI4463 error while configure\r\n");
	}
	SI4463_print_version(G_SI4463);
	RADIO_on(1, 1, 1);
	SI4463_TX_to_RX_transition();

        SI4463_periodic_temperature_check(G_SI4463);
        Int_SI4463.fall(&SI4463_HW_interrupt);

	return 3;
}

#if 0
static void loop100(void)
{
	int i;
	for (i=0; i<100; i++) {
		if ( (is_TDMA_master) && (CONF_master_FDD == 2) ) {
			FDDup_RX_FIFO_dequeue();
		} else {
			radio_RX_FIFO_dequeue(W5500_p1);
		}
#if 0
		Eth_RX_dequeue(W5500_p1); 
#endif
		TDMA_slave_timeout();
#ifdef EXT_SRAM_USAGE
		ext_SRAM_periodic_call();
#endif
		if (is_SRAM_ext == 1) {
			ext_SRAM_periodic_call();
		}
		timer_snapshot = slow_timer.read_us();
		if (timer_snapshot > 666000) {//666000
			slow_timer.reset();
			slow_action_counter++;
			if (slow_action_counter > 2) {slow_action_counter = 0; }
			
			if (slow_action_counter == 0) {
				HMI_periodic_call();
			}
			//debug_counter = 0;

			
			if (slow_action_counter == 1) {//every 2 sec
				signaling_counter++;
				if (signaling_counter >= CONF_signaling_period) {
					signaling_periodic_call();
					signaling_counter = 0;
				}
			}
			
			if (slow_action_counter == 2) {
				DHCP_ARP_periodic_free_table();
#if 0
				W5500_re_configure_periodic_call(W5500_p1);
#endif
			
				temperature_timer++;
				if(temperature_timer > 15) {// 15 every 30 sec
					temperature_timer = 0;
					G_need_temperature_check = 1;
					SI4463_periodic_temperature_check_2();//SI4463_periodic_temperature_check(G_SI4463);
				}
			}
		}
	}
}
#endif

#if 0
static void
init1(void)
{
	int i;
#if 1
	//SI4463_print_version(G_SI4463);//!!!!
	SI4463_get_state(G_SI4463);

	i = SI4463_configure_all();
	if (i == 1) {
		HMI_printf("SI4463 configured\r\n");
        } else {
                HMI_printf("SI4463 error while configure\r\n");
                SI4463_print_version(G_SI4463);
                // wait_ms(5000);
                // if (serial_term_loop() == 0) {//no serial char detected
                //        NVIC_SystemReset(); //reboot
                // }
        }
#endif

	// W5500_initial_configure(W5500_p1);
	wait_ms(2);
#ifdef EXT_SRAM_USAGE
	ext_SRAM_set_mode(SPI_SRAM_p);
	wait_ms(2)
	ext_SRAM_init();
#endif
	TDMA_NULL_frame_init(70);

	SI4463_periodic_temperature_check(G_SI4463);
	Int_SI4463.fall(&SI4463_HW_interrupt);
	if (CONF_radio_default_state_ON_OFF) {
		//TDMA_init_all();
		//SI4463_radio_start();
		RADIO_on(1, 0, 1);//init state, no reconfigure, HMI output
        }
	HMI_printf("ready> ");
	slow_timer.start();
}
#endif

int main()
{
	int i;
#if 0
	stdio_uart_init();
#else
	stdio_uart_init_full(uart_default, 460800, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
#endif
	debug("init_spi()\r\n");
	init_spi();
	debug("littlefs_init()\r\n");
	littlefs_init();	
#if 0
	wait_ms(20);
        is_SRAM_ext = ext_SRAM_detect();

	init_si4463();
	    LED_RX_loc = 1;
        for (i=0; i<7; i++) {
                wait_ms(200);
                LED_RX_loc = !LED_RX_loc;
                LED_connected = !LED_connected;
                SI4463_SDN = !SI4463_SDN;
        }
        LED_RX_loc = 0;
        LED_connected = 0;
        SI4463_SDN = 1;

	debug("NFPR_config_read()\r\n");
	NFPR_config_read(&Random_pin);
#endif
	
	

	init1();

#if 1
	// Initialize tinyusb, lwip, dhcpd and httpd
	if (cyw43_arch_init()) {
		printf("failed to initialise\n");
	}
#else
	lwip_init();
#endif
	tud_setup();
	enchw_init();
	init_wifi();
	bridge_setup();
#if 0
	wait_for_netif_is_up();
	dhcpd_init();
#endif

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	tcp_setup();
	gpio_put(LED_PIN, 1);
#if 0
	SI4463_SDN = 1;
	wait_ms(300);
	SI4463_SDN = 0;
#endif
#if 0
	init_si4463();
	SI4463_SDN = 1;
#endif


	while (true) {
		loop();
		tud_task();
		service_traffic();
		misc_loop();
		cyw43_arch_poll();
		enchw_poll();
	}

	return 0;
}
