#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "W5500_lwip.h"
#include "netif/etharp.h"
#include "netif/bridgeif.h"
#include "common.h"
#include "main.h"
#include "../source/HMI_telnet.h"
#include "../source/Eth_IPv4.h"

static void bridge_setup(void);
static void ip_setup(void);
#define LED_PIN     25
#define NUM_BRIDGE_PORTS 5


struct W5500_channel W5500_channelx[NR_SOCKETS];
static struct netif netif_radio;
static struct netif netif_bridge;
static struct netif *bridge_port[NUM_BRIDGE_PORTS];
static err_t radio_output_raw_fn(struct pbuf *p);

static int verbose;


static void
IP_int2lwip(unsigned long int IP_int, ip4_addr_t *ip)
{
	IP_int2char(IP_int, (unsigned char *)ip);
}

struct W5500_channel *
W5500_chan(int idx)
{
	return &W5500_channelx[(idx-1)/4];
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
W5500_enqueue_pbuf(struct W5500_channel *c, struct pbuf *p)
{
	if (c->pbuf) {
		pbuf_chain(c->pbuf, p);
	} else {
		c->pbuf=p;
	}
}

void
W5500_enqueue(struct W5500_channel *c, unsigned char *data, int size)
{
	struct pbuf *p=pbuf_alloc(PBUF_RAW, size, PBUF_RAM);
	memcpy(p->payload, data, size);
	W5500_enqueue_pbuf(c, p);
}

int
W5500_next_size(struct W5500_channel *c)
{
	if (!c->pbuf)
		return 0;
	return c->pbuf->len;
}

static void
W5500_update_int(void)
{
	struct W5500_channel *c=W5500_chan(1);
	Int_W5500.setstate(W5500_next_size(c) == 0);
}


err_t
W5500_transmit(struct W5500_channel *c, unsigned char *buffer, int len)
{
	err_t err = ERR_OK;
	if (c->udp) {
		struct pbuf *pbuf=pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);
		memcpy(pbuf->payload, buffer, len);
		if (verbose)
			printf("udp_sendto_if\r\n");
		err = udp_sendto_if(c->udp, pbuf, IP_ADDR_BROADCAST, 68, &netif_bridge);
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
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
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
	if (verbose)
		debug("W5500_udp_recv\r\n");
	if (c && p->len && p->payload) {
		W5500_enqueue(c, (unsigned char *)p->payload, p->len);
	}
	pbuf_free(p);
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
	case 0x0002:
		Int_W5500.setstate(1);
		break;
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
	if (sock_nb == 1) {
		struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
		debug("raw send %d %d\r\n",size,send_mac);
		memcpy(p->payload, data, size);
		radio_output_raw_fn(p);
		pbuf_free(p);
	}
	else
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
	if (!sock_nb)
		debug("received_size RAW\r\n");
	if (!c)
		return 0;
	return W5500_next_size(c);
}

void
W5500_re_configure(void)
{
	ip4_addr_t addr;
	debug("W5500_re_configure\r\n");
}

int
W5500_read_MAC_pckt (W5500_chip* SPI_p_loc, uint8_t sock_nb, unsigned char* data)
{
	int i,len;
	struct W5500_channel *c=W5500_chan(sock_nb);
	if (!c)
		return 0;
	struct pbuf *p=W5500_dequeue(c);
	if (!p)
		return 0;
	len=p->len;
	data[0]=0;
	data[1]=0;
	memcpy(data+2,(char *)p->payload, len);
        pbuf_free(p);
	return len+2;
}

void
W5500_initial_configure(W5500_chip* SPI_p_loc)
{
	Int_W5500.setstate(1);
	bridge_setup();
	ip_setup();
}

void
W5500_re_configure_periodic_call(W5500_chip* SPI_p_loc)
{
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
		HMI_cprintf(ctx,"%lu.%lu.%lu.%lu ", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24);
		ip_addr=netif->netmask.addr;
		HMI_cprintf(ctx,"%lu.%lu.%lu.%lu ", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24);
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

static err_t radio_input_fn(struct pbuf *p, struct netif *netif)
{
	struct W5500_channel *c=W5500_chan(1);
	struct netif *netif_input=&netif_radio;
	// debug("radio_input_fn\r\n");
	W5500_enqueue(c, (unsigned char *)p->payload, p->len);
	W5500_update_int();
#if 0
	if (netif_input->input(p, netif_input) != ERR_OK) 
		pbuf_free(p);
#endif
	return ERR_OK;
}

static err_t radio_output_raw_fn(struct pbuf *p)
{
	struct netif *netif=&netif_radio;
	if (!p)
		return ERR_OK;
	return netif->input(p, netif);
}

static err_t radio_linkoutput_fn(struct netif *netif, struct pbuf *p)
{
	if (verbose)
		debug("radio_linkoutput_fn\r\n");
	struct W5500_channel *c=W5500_chan(1);
	// debug("radio_input_fn\r\n");
	W5500_enqueue(c, (unsigned char *)p->payload, p->len);
	W5500_update_int();
#if 0
	ethernet_input(p, netif);
#endif
	return ERR_OK;
}

static err_t radio_netif_init_cb(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	int i;
	netif->mtu = 1500;
	netif->hwaddr_len = 6;
	for (i = 0 ; i < 6 ; i++)
		netif->hwaddr[i]=CONF_modem_MAC[i];
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->state = NULL;
	netif->name[0] = 'r';
	netif->name[1] = 'd';
	netif->linkoutput = radio_linkoutput_fn;
	netif->output = etharp_output;
	return ERR_OK;
}

static err_t bridge_input_fn(struct pbuf *p, struct netif *netif)
{
	int i;
	if (verbose)
		printf("bridge_input_fn %c%c %p\r\n",netif->name[0],netif->name[1],p);
	for (i = 0 ; i < NUM_BRIDGE_PORTS; i++) {
		struct netif *dest=bridge_port[i];
		/* avoid loopback and feedback from modem ip to radio */
		if (dest && dest != netif && (netif != &netif_bridge || dest != &netif_radio)) {
			if (verbose)
				printf("bridge linkoutput %c%c %p\r\n",bridge_port[i]->name[0],bridge_port[i]->name[1], p);
			bridge_port[i]->linkoutput(bridge_port[i], p);
		}
	}
	if (netif != &netif_bridge)
		ethernet_input(p, &netif_bridge);
	return ERR_OK;
}

static err_t bridge_linkoutput_fn(struct netif *netif, struct pbuf *p)
{
	if (verbose)
		printf("bridge_linkoutput_fn %c%c %p\r\n",netif->name[0],netif->name[1],p);
	return bridge_input_fn(p, netif);
}

static void bridge_add_if(struct netif *portif)
{
	int i;
	for (i = 0 ; i < NUM_BRIDGE_PORTS; i++) {
		if (!bridge_port[i]) {
			bridge_port[i]=portif;
			netif_clear_flags(portif, NETIF_FLAG_ETHARP);
			portif->input = bridge_input_fn;
			return;
		}
	}
}

static err_t bridge_init_cb(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	int i;
	netif->mtu = 1500;
	netif->hwaddr_len = 6;
	for (i = 0 ; i < 6 ; i++)
		netif->hwaddr[i]=CONF_modem_MAC[i];
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->state = NULL;
	netif->name[0] = 'b';
	netif->name[1] = 'r';
	netif->linkoutput = bridge_linkoutput_fn;
	netif->output = etharp_output;
	return ERR_OK;
}

void
ip_setup(void)
{
	ip4_addr_t addr;
	struct netif *netif=&netif_bridge;
	IP_int2lwip(LAN_conf_applied.LAN_modem_IP, &addr);
	netif_set_ipaddr(netif, &addr);
	IP_int2lwip(LAN_conf_applied.LAN_subnet_mask, &addr);
	netif_set_netmask(netif, &addr);
#if 0
	if (CONF_radio_IP_size_internal) {
		IP_int2lwip(LAN_conf_applied.DHCP_range_start+LAN_conf_applied.DHCP_range_size+CONF_radio_IP_size_internal-1, &addr);
		netif_set_ipaddr(&radio, &addr);
		IP_int2lwip(0xffffffff, &addr);
		netif_set_netmask(&radio, &addr);
	}
#endif
}

void
bridge_setup(void)
{
	err_t err;
	struct netif *netif;
	netif=netif_add_noaddr(&netif_radio,  NULL, radio_netif_init_cb, ethernet_input);
	netif=netif_add_noaddr(&netif_bridge, NULL, bridge_init_cb, ethernet_input);
	bridge_add_if(&netif_radio);
	bridge_add_if(&netif_eth);
#if 0
	debug("radio %p\r\n",netif);
	netif=netif_add(&bridge, &ipaddr, &netmask, &gateway, &mybridge_initdata, bridgeif_init, ethernet_input);
	debug("bridge %p\r\n",netif);
	err=bridgeif_add_port(&bridge, &radio);
	debug("bridge add radio %d\r\n",err);
	err=bridgeif_add_port(&bridge, &netif_data);
	debug("bridge add usb %d\r\n",err);
	bridge.flags |= NETIF_FLAG_UP;
#endif
}
