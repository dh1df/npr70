#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "W5500_lwip.h"
#include "netif/etharp.h"
#include "netif/bridgeif.h"
#include "common.h"
#include "../source/HMI_telnet.h"

#define LED_PIN     25

struct W5500_channel W5500_channelx[NR_SOCKETS];
static struct netif bridge,radio;

struct W5500_channel *
W5500_chan(int idx)
{
	return &W5500_channelx[idx];
}

struct pbuf *
W5500_dequeue(struct W5500_channel *c)
{
	struct pbuf *ret=c->pbuf;
	if (ret) {
		c->pbuf=pbuf_dechain(c->pbuf);
	}
	return ret;
}

void
W5500_enqueue(struct W5500_channel *c, unsigned char *data, int size)
{
	struct pbuf *p=pbuf_alloc(PBUF_RAW, size, PBUF_RAM);
	memcpy(p->payload, data, size);
	if (c->pbuf) {
		pbuf_chain(c->pbuf, p);
	} else {
		c->pbuf=p;
	}
}

int
W5500_next_size(struct W5500_channel *c)
{
	if (!c->pbuf)
		return 0;
	return c->pbuf->len;
}

err_t
W5500_transmit(struct W5500_channel *c, unsigned char *buffer, int len)
{
	err_t err = ERR_OK;
	if (c->udp) {
		struct pbuf *pbuf=pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);
		memcpy(pbuf->payload, buffer, len);
		err = udp_sendto_if(c->udp, pbuf, IP_ADDR_BROADCAST, 68, &bridge);
		pbuf_free(pbuf);
	} else {
		err = tcp_write(c->conn, buffer, len, TCP_WRITE_FLAG_COPY);
		if (!err) {
			err=tcp_output(c->conn);
		}
	}
	return err;
}

static void
W5500_error_handler(void* arg, err_t err)
{
	printf("tcpErrorHandler()=%d\n", err);

	for(;;)  { gpio_xor_mask(1 << LED_PIN);  sleep_ms(250); }
}

static err_t
W5500_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct W5500_channel *c=(struct W5500_channel *)arg;
	gpio_put(LED_PIN, 0);

	if (p == NULL) {
		tcp_close(tpcb);
		c->conn=NULL;
		return ERR_ABRT;
	} else {
		if (p->len && p->payload) {
			W5500_enqueue(c, (unsigned char *)p->payload, p->len);
		}
	}

	return 0;
}

err_t
W5500_tcp_sent(void* arg, struct tcp_pcb *pcb, u16_t len)
{
	struct W5500_channel *c=(struct W5500_channel *)arg;
	gpio_put(LED_PIN, 0);
#if 0
	tcp_output(c->conn);
#endif
	return 0;
}


err_t
W5500_poll(void *arg, struct tcp_pcb *tpcb)
{
	struct W5500_channel *c=(struct W5500_channel *)arg;
	err_t ret_err = ERR_OK;

	tcp_output(c->conn);

	return ret_err;
}




err_t
W5500_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	struct W5500_channel *c=(struct W5500_channel *)arg;
	if ((err != ERR_OK) || (newpcb == NULL)) {
		return ERR_VAL;
	}
	if (c->conn) {
		const char *str="Another session is already in use\r\n";
		tcp_write(newpcb, str, strlen(str), 0);
		tcp_output(newpcb);
		tcp_close(newpcb);
	} else {
		c->conn=newpcb;
		// tcp_setprio(newpcb, TCP_PRIO_MIN);

		/* pass newly allocated es to our callbacks */
		tcp_arg(newpcb, c);
		tcp_err(newpcb, W5500_error_handler);
		tcp_recv(newpcb, W5500_tcp_recv);
		tcp_sent(newpcb, W5500_tcp_sent);
		tcp_poll(newpcb, W5500_poll, 0);
	}

  	return ERR_OK;
}

void W5500_udp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	struct W5500_channel *c=(struct W5500_channel *)arg;
	if (c && p->len && p->payload) {
		W5500_enqueue(c, (unsigned char *)p->payload, p->len);
	}
}

uint8_t
W5500_read_byte(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr)
{
	struct W5500_channel *c=W5500_chan(bloc_addr);

	if (!c)
		return 0;

	switch(W5500_addr) {
	case W5500_Sn_SR:
		return c->conn?W5500_SOCK_ESTABLISHED:W5500_SOCK_CLOSED;
	}
	return 0;
}

void
W5500_write_byte(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr, unsigned char data)
{
	struct W5500_channel *c=W5500_chan(bloc_addr);

	if (!c)
		return;
	switch(W5500_addr) {
	case W5500_Sn_CR:
		if (data == 8 || data == 0x10) {
			tcp_close(c->conn);
			c->conn=NULL;
		}
		break;
	}
}

void
W5500_read_long(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr, unsigned char* RX_data, int RX_size)
{
	struct W5500_channel *c=W5500_chan(bloc_addr);
	if (!c)
		return;
	switch(W5500_addr) {
	case W5500_Sn_DIPR0:
		if (RX_size == 7) {
			ip_addr_t *ipaddr=&c->conn->remote_ip;
			RX_data[0]=0;
			RX_data[1]=0;
			RX_data[2]=0;
			RX_data[3]=ip4_addr1(ipaddr);
			RX_data[4]=ip4_addr2(ipaddr);
			RX_data[5]=ip4_addr3(ipaddr);
			RX_data[6]=ip4_addr4(ipaddr);
		}
	}
}

void
W5500_write_TX_buffer(W5500_chip* SPI_p_loc, uint8_t sock_nb, unsigned char* data, int size, int send_mac) 
{
	struct W5500_channel *c=W5500_chan(sock_nb);
	if (!c)
		return;
#if 0
	W5500_enqueue(c, TX, data, size);
#if 1
	W5500_transmit(c);
#endif
#else
	if (sock_nb == TELNET_SOCKET) {
		fwrite(data, size, 1, stdout);
		fflush(stdout);
	}
	W5500_transmit(c, data, size);
#endif
}

void
W5500_read_RX_buffer(W5500_chip* SPI_p_loc, uint8_t sock_nb, uint8_t* data, int size)
{
	struct W5500_channel *c=W5500_chan(sock_nb);
	if (!c)
		return;
	struct pbuf *p=W5500_dequeue(c);
#if 0
	debug("read_RX %p %d\r\n",p,size);
#endif
	if (p) {
		int hlen=3;
		if (size > p->len+hlen)
			size=p->len+hlen;
		data[0]=0;
		data[1]=0;
		data[2]=0;
		memcpy(data+hlen, p->payload, size);
                pbuf_free(p);
	}
}

uint32_t
W5500_read_UDP_pckt(W5500_chip* SPI_p_loc, uint8_t sock_nb, unsigned char* data, uint32_t size)
{
	struct W5500_channel *c=W5500_chan(sock_nb);
	if (!c)
		return 0;
	struct pbuf *p=W5500_dequeue(c);
#if 0
	debug("read_RX %p %d\r\n",p,size);
#endif
	if (p) {
		int hlen=8;
		if (size > p->len+hlen || size == 0)
			size=p->len+hlen;
		data[0]=0;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		data[4]=0;
		data[5]=0;
		data[6]=0;
		data[7]=0;
		memcpy(data+hlen, p->payload, size);
                pbuf_free(p);
		return size+hlen;
	} else
		return 0;
}


uint16_t
W5500_read_received_size(W5500_chip* SPI_p_loc, uint8_t sock_nb)
{
	struct W5500_channel *c=W5500_chan(sock_nb);
	if (!c)
		return 0;
	return W5500_next_size(c);
}

void
W5500_re_configure(void)
{
	debug("W5500_re_configure\r\n");
}

int
W5500_read_MAC_pckt (W5500_chip* SPI_p_loc, uint8_t sock_nb, unsigned char* data)
{
	debug("W5500_read_MAC_pckt\r\n");
	return 0;
}

int
cmd_display_net(struct context *ctx)
{
	struct netif *netif=netif_list;
	u32_t ip_addr;
	while (netif) {
		unsigned char *h=netif->hwaddr;
		HMI_cprintf(ctx,"%p %s %d ",netif,netif->name,netif->num);
		if (netif->flags & NETIF_FLAG_UP) {
			HMI_printf("UP ");
		}
		if (netif->flags & NETIF_FLAG_LINK_UP) {
			HMI_printf("LINK_UP ");
		}
		if (netif->flags & NETIF_FLAG_ETHARP) {
			HMI_printf("ETHARP ");
		}
		if (netif->flags & NETIF_FLAG_ETHERNET) {
			HMI_printf("ETHERNET ");
		}
		ip_addr=netif->ip_addr.addr;
		HMI_cprintf(ctx,"%lu.%lu.%lu.%lu ", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24)
		ip_addr=netif->netmask.addr;
		HMI_cprintf(ctx,"%lu.%lu.%lu.%lu ", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24)
		HMI_cprintf(ctx,"%d %02x:%02x:%02x:%02x:%02x:%02x\r\n", netif->hwaddr_len, h[0], h[1], h[2], h[3], h[4], h[5]);
		
		netif=netif->next;
	}
	return 2;
}

extern "C" {

void
debug_mac(unsigned char *h)
{
	debug("%02x:%02x:%02x:%02x:%02x:%02x", h[0], h[1], h[2], h[3], h[4], h[5]);
}

void
debug_ip(u32_t ip_addr)
{
	debug("%lu.%lu.%lu.%lu", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24);
}

void
debug_udp(unsigned char *d)
{
	debug("%02x%02x %02x%02x",d[0],d[1],d[2],d[3]);
}

void
debug_icmp(unsigned char *d)
{
	debug("%02x %02x %02x%02x %02x%02x",d[0],d[1],d[4],d[5],d[6],d[7]);
}

void
debug_iph(unsigned char *d)
{
	int ihl=4;
	debug("%d.%d.%d.%d",d[12],d[13],d[14],d[15]);
	debug(" ");
	debug("%d.%d.%d.%d",d[16],d[17],d[18],d[19]);
	debug(" %02x ",d[9]);
	if (d[9] == 1) {
		debug_icmp(d+ihl*5);
	}
	if (d[9] == 17) {
		debug_udp(d+ihl*5);
	}
}

void
debug_arp(unsigned char *d)
{
	debug("ARP %02x%02x %02x%02x %02x %02x %02x%02x ",d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]);
	debug_mac(d+8);
	debug(" ");
	debug("%d.%d.%d.%d",d[14],d[15],d[16],d[17]);
	debug(" ");
	debug_mac(d+18);
	debug(" ");
	debug("%d.%d.%d.%d",d[24],d[25],d[26],d[27]);
	debug("\r\n");
}	

void
debug_pbuf(const char *id, struct pbuf *p)
{
	unsigned char *d=(unsigned char *)p->payload;
	unsigned int e=(d[12]<<8)+d[13];
	debug("%s(%d) ",id,p->len);
	debug_mac(d);
	debug(" ");
	debug_mac(d+6);
	debug(" %04x ",e);
	if (e == 0x800) {
		debug_iph(d+14);
	}
	if (e == 0x806) {
		debug_arp(d+14);
	}
	debug("\r\n");
}
}

static err_t radio_linkoutput_fn(struct netif *netif, struct pbuf *p)
{
#if 0
#if 0
	debug_pbuf("radio_out",p);
#else
	debug("radio_out\r\n");
#endif
#endif
	return ERR_OK;
}

static err_t radio_output_fn(struct netif *netif, struct pbuf *p, const ip_addr_t *addr)
{
	debug("radio_output_fn\r\n");
	return etharp_output(netif, p, addr);
}

static err_t radio_netif_init_cb(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	debug("radio_netif_init_cb\r\n");
	netif->mtu = 1500;
	netif->hwaddr_len = 6;
	netif->hwaddr[0]=0x1;
	netif->hwaddr[1]=0x2;
	netif->hwaddr[2]=0x3;
	netif->hwaddr[3]=0x4;
	netif->hwaddr[4]=0x5;
	netif->hwaddr[5]=0x7;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->state = NULL;
	netif->name[0] = 'r';
	netif->name[1] = 'd';
	netif->linkoutput = radio_linkoutput_fn;
	netif->output = radio_output_fn;
	return ERR_OK;
}

static const ip_addr_t ipaddr  = IPADDR4_INIT_BYTES(192, 168, 0, 253);
static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t gateway = IPADDR4_INIT_BYTES(192, 168, 0, 65);
static bridgeif_initdata_t mybridge_initdata = BRIDGEIF_INITDATA1(4, 512, 16, ETH_ADDR(0, 1, 2, 3, 4, 5));
extern struct netif netif_data;

void
bridge_setup(void)
{
	err_t err;
	struct netif *netif;
	netif=netif_add_noaddr(&radio, NULL, radio_netif_init_cb, ip_input);
	debug("radio %p\r\n",netif);
	netif=netif_add(&bridge, &ipaddr, &netmask, &gateway, &mybridge_initdata, bridgeif_init, ethernet_input);
	debug("bridge %p\r\n",netif);
	err=bridgeif_add_port(&bridge, &radio);
	debug("bridge add radio %d\r\n",err);
	err=bridgeif_add_port(&bridge, &netif_data);
	debug("bridge add usb %d\r\n",err);
	bridge.flags |= NETIF_FLAG_UP;
}
