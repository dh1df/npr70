#include "enchw.h"
#include "enc28j60.h"
#include "npr70piconfig.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include <netif/etharp.h>
#include "netif/mchdrv.h"

static enc_device_t dev;

struct netif netif_data;
static const ip_addr_t ipaddr  = IPADDR4_INIT_BYTES(192, 168, 6, 1);
static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t gateway = IPADDR4_INIT_BYTES(192, 168, 6, 2);


void enchw_init(void)
{
#if 0
	debug("enc_setup_basic %d\r\n",enc_setup_basic(&dev));
	debug("enc_bist_manual %d\r\n",enc_bist_manual(&dev));
#endif
	struct netif *netif=&netif_data;
	netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, mchdrv_init, ip_input);
}

void enchw_setup(enchw_device_t *dev)
{
	debug("enchw_setup\r\n");
}

void enchw_select(enchw_device_t *dev)
{
	asm volatile("nop \n nop \n nop");
	gpio_put(PIN_CS, 0);
	asm volatile("nop \n nop \n nop");
}

void enchw_unselect(enchw_device_t *dev)
{
	asm volatile("nop \n nop \n nop");
	gpio_put(PIN_CS, 1);
	asm volatile("nop \n nop \n nop");
}

uint8_t enchw_exchangebyte(enchw_device_t *dev, uint8_t data)
{
	uint8_t ret;
	asm volatile("nop \n nop \n nop");
	spi_write_read_blocking(SPI_PORT, &data, &ret, 1);
	asm volatile("nop \n nop \n nop");
#if 0
	debug("enchw_exchangebyte %d %d\r\n",data, ret);
#endif
	return ret;
}