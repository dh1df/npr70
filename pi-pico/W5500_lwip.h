#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "../source/W5500.h"

#define NR_SOCKETS 16
enum W5500_dir {
	RX=0,
	TX=1
};
extern struct W5500_channel {
	struct tcp_pcb *conn;
	struct pbuf *pbuf[2];
} W5500_channel[NR_SOCKETS];

struct pbuf *W5500_dequeue(struct W5500_channel *c, enum W5500_dir dir);
void W5500_enqueue(struct W5500_channel *c, enum W5500_dir dir, unsigned char *data, int size);
err_t W5500_transmit(struct W5500_channel *c);
err_t W5500_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
