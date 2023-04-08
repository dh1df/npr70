#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "../source/W5500.h"

#define NR_SOCKETS 16
extern struct W5500_channel {
	struct tcp_pcb *tcp;
	struct udp_pcb *udp;
	struct tcp_pcb *conn;
	struct pbuf *pbuf;
} W5500_channelx[NR_SOCKETS];

struct pbuf *W5500_dequeue(struct W5500_channel *c);
void W5500_enqueue(struct W5500_channel *c, unsigned char *data, int size);
err_t W5500_transmit(struct W5500_channel *c, unsigned char *buffer, int len);
err_t W5500_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
struct W5500_channel *W5500_chan(int idx);
void W5500_udp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
