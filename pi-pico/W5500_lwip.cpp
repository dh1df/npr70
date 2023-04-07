#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "W5500_lwip.h"
#include "../source/HMI_telnet.h"

#define LED_PIN     25

struct W5500_channel W5500_channel[NR_SOCKETS];
static struct netif bridge,radio;

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
	err = tcp_write(c->conn, buffer, len, TCP_WRITE_FLAG_COPY);
	if (!err) {
		err=tcp_output(c->conn);
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


uint8_t
W5500_read_byte(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr)
{
	struct W5500_channel *c=&W5500_channel[bloc_addr];

	switch(W5500_addr) {
	case W5500_Sn_SR:
		return c->conn?W5500_SOCK_ESTABLISHED:W5500_SOCK_CLOSED;
	}
	return 0;
}

void
W5500_write_byte(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr, unsigned char data)
{
	struct W5500_channel *c=&W5500_channel[bloc_addr];
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
	struct W5500_channel *c=&W5500_channel[bloc_addr];
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
	struct W5500_channel *c=&W5500_channel[sock_nb];
#if 0
	W5500_enqueue(c, TX, data, size);
#if 1
	W5500_transmit(c);
#endif
#else
	fwrite(data, size, 1, stdout);
	fflush(stdout);
	W5500_transmit(c, data, size);
#endif
}

void
W5500_read_RX_buffer(W5500_chip* SPI_p_loc, uint8_t sock_nb, uint8_t* data, int size)
{
	struct W5500_channel *c=&W5500_channel[sock_nb];
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

uint16_t
W5500_read_received_size(W5500_chip* SPI_p_loc, uint8_t sock_nb)
{
	struct W5500_channel *c=&W5500_channel[sock_nb];
	return W5500_next_size(c);
}

void
net_display(void)
{
	struct netif *netif=netif_list;
	u32_t ip_addr;
	while (netif) {
		HMI_printf("%p %s %d ",netif,netif->name,netif->num);
		ip_addr=netif->ip_addr.addr;
		HMI_printf("%lu.%lu.%lu.%lu ", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24)
		ip_addr=netif->netmask.addr;
		HMI_printf("%lu.%lu.%lu.%lu\r\n", ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, ip_addr >> 24)
		netif=netif->next;
	}

}

void
bridge_setup(void)
{
#if 0
	err_t err;
	err=bridgeif_init(&bridge);
	debug("bridgeif_init %d\r\n",err);
#endif
}
