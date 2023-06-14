#include <string.h>
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "common.h"
#include "npr70piextra.h"
#include "../source/HMI_telnet.h"

static unsigned char seq;
static int tracing;
#define NUM_PBUF 8
static int rx,tx,err_nomem,err_overflow;
static struct pbuf *tracing_pbufs[NUM_PBUF];
#define HEADER_SIZE (6+6+2+1+1+4)

static struct pbuf *trace_alloc(int type, unsigned int us, int size)
{
	struct pbuf *p;
	unsigned char *pl;

	if (!tracing)
		return NULL;
	seq++;
	if (tracing_pbufs[rx]) {
		err_overflow++;
		return NULL;
	}
	p=pbuf_alloc(PBUF_RAW, size + HEADER_SIZE, PBUF_POOL);
	if (!p) {
		err_nomem++;
		return NULL;
	}
	pl=p->payload;
	memset(pl, 0, 6);
	pl+=6;
	memset(pl, 0, 6);
	pl+=6;
	*pl++=0xff;
	*pl++=0xff;
	*pl++=type;
	*pl++=seq;
	memcpy(pl, &us, 4);
	return p;
}

static void trace_add(struct pbuf *p)
{
	tracing_pbufs[rx] = p;
	rx++;
	if (rx >= NUM_PBUF)
		rx=0;
}

void trace_rx_radio(unsigned int us, int initial, unsigned char *data, int mask, int offset, int len)
{
	struct pbuf *p;
	unsigned char *pl;

	p=trace_alloc(initial ? 0:1, us, len);
	if (!p) 
		return;
	pl=p->payload+HEADER_SIZE;
	while(len) {
		*pl++=data[offset & mask];
		offset++;
		len--;
	}
	trace_add(p);
}

void trace_tx_radio(unsigned int us, int initial, unsigned char *data, int len)
{
	struct pbuf *p;
	unsigned char *pl;

	p=trace_alloc(initial ? 2:3, us, len);
	if (!p) 
		return;
	pl=p->payload+HEADER_SIZE;
	memcpy(pl, data, len);
	trace_add(p);
}

int cmd_trace(struct context *ctx)
{
	if (!strcmp(ctx->s1,"on"))
		tracing=1;
	else if (!strcmp(ctx->s1,"off"))
		tracing=0;
	else {
		if (strcmp(ctx->s1,"status"))
			HMI_cprintf(ctx,"Use on|off|status\r\n");
		HMI_cprintf(ctx,"Tracing is %s, nomem %d, overflow %d\r\n", tracing?"on":"off", err_nomem, err_overflow);
	}
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
