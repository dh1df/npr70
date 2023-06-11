#include <string.h>
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "common.h"
#include "npr70piextra.h"

static unsigned char seq;
static int tracing;
#define NUM_PBUF 8
static int rx,tx;
static struct pbuf *tracing_pbufs[NUM_PBUF];

void trace_rx_radio(unsigned int us, int initial, unsigned char *data, int mask, int offset, int len)
{
	struct pbuf *p;
	unsigned char *pl;

	if (!tracing)
		return;
	if (tracing_pbufs[rx]) {
		printf("too slow\r\n");
		return;
	}
	printf("trace %d %d\r\n",initial,len);
	seq++;
	p=pbuf_alloc(PBUF_RAW, len+6+6+2+1+1+4, PBUF_POOL);
	if (!p) {
		printf("out of memory\r\n");
		return;
	}
	pl=p->payload;
	memset(pl, 0, 6);
	pl+=6;
	memset(pl, 0, 6);
	pl+=6;
	*pl++=0xff;
	*pl++=0xff;
	*pl++=initial?0:1;
	*pl++=seq;
	memcpy(pl, &us, 4);
	pl+=4;
	while(len) {
		*pl++=data[offset & mask];
		offset++;
		len--;
	}
	tracing_pbufs[rx] = p;
	rx++;
	if (rx >= NUM_PBUF)
		rx=0;
}

int cmd_trace(struct context *ctx)
{
	tracing=1;
	return 3;
}

void trace_loop(void)
{
	if (tracing_pbufs[tx]) {
		netif_usb.linkoutput(&netif_usb, tracing_pbufs[tx]);
		pbuf_free(tracing_pbufs[tx]);
		tracing_pbufs[tx] = NULL;
		tx++;
		if (tx >= NUM_PBUF)
			tx=0;
	}
}
