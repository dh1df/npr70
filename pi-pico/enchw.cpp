#include "mbed.h"
#include "npr70piconfig.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "netif/etharp.h"
#include "../source/global_variables.h"
#include "common.h"

extern "C" {
#include "enchw.h"
#include <pico/enc28j60/enc28j60.h>
#include <pico/enc28j60/ethernetif.h>
};

struct netif netif_eth;

unsigned char CONF_ethernet_MAC[6];

struct enc28j60 enc28j60 = {
	.spi = spi0,
        .cs_pin = ENC_PIN_CS,
};



void enchw_poll(void)
{
	if (gpio_get(ENC_PIN_INT))
		return;
	enc28j60_isr_begin(&enc28j60);
	uint8_t flags = enc28j60_interrupt_flags(&enc28j60);

	if (flags & ENC28J60_PKTIF) {
		struct netif *netif=&netif_eth;
		struct pbuf *packet = low_level_input(netif);
		if (packet != NULL) {
			if(netif->input(packet, netif) != ERR_OK) {
                                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
				pbuf_free(packet);
                        }
		}
	}

	if (flags & ENC28J60_TXERIF) {
		LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: transmit error\n"));
	}

	if (flags & ENC28J60_RXERIF) {
		LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: receive error\n"));
	}

	enc28j60_interrupt_clear(&enc28j60, flags);
	enc28j60_isr_end(&enc28j60);
}

void enchw_init(void)
{
	struct netif *netif=&netif_eth;
	memcpy(&enc28j60.mac_address, CONF_ethernet_MAC, 6);
	netif = netif_add_noaddr(netif, &enc28j60, ethernetif_init, netif_input);
        netif_set_link_up(netif);
	enc28j60_interrupts(&enc28j60, ENC28J60_PKTIE | ENC28J60_TXERIE | ENC28J60_RXERIE);
}
