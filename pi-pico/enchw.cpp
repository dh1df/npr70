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

struct netif netif_data_eth;
static const ip_addr_t ipaddr  = IPADDR4_INIT_BYTES(192, 168, 6, 1);
static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t gateway = IPADDR4_INIT_BYTES(192, 168, 6, 2);

struct enc28j60 enc28j60 = {
	.spi = spi0,
        .cs_pin = ENC_PIN_CS,
};



void enchw_poll(void)
{
	enc28j60_isr_begin(&enc28j60);
	uint8_t flags = enc28j60_interrupt_flags(&enc28j60);

	if (flags & ENC28J60_PKTIF) {
		struct pbuf *packet = low_level_input(&netif_data_eth);
		if (packet != NULL) 
			netif_data_eth.input(packet, &netif_data_eth);
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
	struct netif *netif=&netif_data_eth;
	int i;
	for (i = 0 ; i < 6 ; i++)
		enc28j60.mac_address[i]=CONF_modem_MAC[i];
	netif_add(netif, &ipaddr, &netmask, &gateway, &enc28j60, ethernetif_init, netif_input);
        netif_set_up(netif);
        netif_set_link_up(netif);

}
