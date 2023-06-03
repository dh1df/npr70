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
#include "enc28j60.h"
#include "netif/mchdrv.h"
};

static enc_device_t dev;

struct netif netif_data_eth;
static const ip_addr_t ipaddr  = IPADDR4_INIT_BYTES(192, 168, 6, 1);
static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t gateway = IPADDR4_INIT_BYTES(192, 168, 6, 2);


void enchw_poll(void)
{
	mchdrv_poll(&netif_data_eth);	
}

void enchw_init(void)
{
#if 0
	debug("enc_setup_basic %d\r\n",enc_setup_basic(&dev));
	debug("enc_bist_manual %d\r\n",enc_bist_manual(&dev));
	return 0;
#endif
	struct netif *netif=&netif_data_eth;
	int i;

	netif->name[0] = 'e';
        netif->name[1] = 'n';
        netif->hwaddr_len = 6;
	for (i = 0 ; i < 6 ; i++)
		netif->hwaddr[i]=CONF_modem_MAC[i];
	netif->hwaddr[3]++;
	netif = netif_add(netif, &ipaddr, &netmask, &gateway, &dev, mchdrv_init, ethernet_input);
	netif->flags |= NETIF_FLAG_UP | NETIF_FLAG_ETHERNET;
}

void enchw_setup(enchw_device_t *dev)
{
	debug("enchw_setup\r\n");
}

void enchw_select(enchw_device_t *dev)
{
	asm volatile("nop \n nop \n nop");
	gpio_put(ENC_PIN_CS, 0);
	asm volatile("nop \n nop \n nop");
}

void enchw_unselect(enchw_device_t *dev)
{
	asm volatile("nop \n nop \n nop");
	gpio_put(ENC_PIN_CS, 1);
	asm volatile("nop \n nop \n nop");
}

uint8_t enchw_exchangebyte(enchw_device_t *dev, uint8_t data)
{
	uint8_t ret;
	asm volatile("nop \n nop \n nop");
	spi_write_read_blocking(ENC_PORT_SPI, &data, &ret, 1);
	asm volatile("nop \n nop \n nop");
#if 0
	debug("enchw_exchangebyte %d %d\r\n",data, ret);
#endif
	return ret;
}
