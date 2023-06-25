#include "mbed.h"
#include "../source/global_variables.h"
#include "../source/HMI_telnet.h"
#include "npr70piextra.h"
#include "lwip/dns.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"


static void cmd_ping_send(struct ping_context *pctx);

static int cmd_ping_size=32;
static int cmd_ping_id=0xbeef;

struct ping_context {
	enum retcode done;
	struct raw_pcb *pcb;
	uint16_t seq_num;
	struct context *ctx;
	ip_addr_t addr;
	int start;
} cmd_ping_context;

static void cmd_ping_exit(struct ping_context *pctx, enum retcode ret)
{
	// debug("raw_remove %p %p\r\n",pctx, pctx->pcb);
	raw_remove(pctx->pcb);
	pctx->pcb = NULL;
	pctx->done = ret;
}


static void cmd_ping_found(const char *name, const ip_addr_t *ipaddr, void *arg)
{
	struct ping_context *pctx=(struct ping_context *)arg;
	HMI_cprintf(pctx->ctx,"Got %s\r\n", ipaddr_ntoa(ipaddr));
	cmd_ping_send(pctx);
}

static u8_t cmd_ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
	struct ping_context *pctx=(struct ping_context *)arg;
	unsigned int len=p->tot_len;
	struct ip_hdr *ip = (struct ip_hdr *)p->payload;
	int ttl=ip->_ttl;
	// debug("cmd_ping_recv\r\n");
	if ((len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) && pbuf_remove_header(p, PBUF_IP_HLEN) == 0) {
		struct icmp_echo_hdr *icmp_echo = (struct icmp_echo_hdr *)p->payload;
		float delta=(GLOBAL_timer.read_us()-pctx->start)/1000.0;
		if (icmp_echo-> id == cmd_ping_id)
			HMI_cprintf(pctx->ctx, "%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f\r\n", len, ipaddr_ntoa(addr), lwip_ntohs(icmp_echo->seqno), ttl, delta);
		cmd_ping_exit(pctx, RET_OK_PROMPT);
		return 1;
	}
	return 0;
}

static void cmd_ping_send(struct ping_context *pctx)
{
	struct pbuf *p;
	uint16_t size = sizeof(struct icmp_echo_hdr) + cmd_ping_size;
	int i;

	// debug("cmd_ping_send\r\n");
	p = pbuf_alloc(PBUF_IP, size, PBUF_POOL);
	if (p) {
		struct icmp_echo_hdr *icmp_echo = (struct icmp_echo_hdr *)p->payload;
		ICMPH_TYPE_SET(icmp_echo, ICMP_ECHO);
		ICMPH_CODE_SET(icmp_echo, 0);
		icmp_echo->chksum = 0;
		icmp_echo->id     = cmd_ping_id;
		icmp_echo->seqno  = lwip_htons(++(pctx->seq_num));

		for(i = 0; i < cmd_ping_size; i++) 
			((char*)icmp_echo)[sizeof(struct icmp_echo_hdr) + i] = (char)i;

		icmp_echo->chksum = inet_chksum(icmp_echo, size);
		pctx->start=GLOBAL_timer.read_us();
		raw_sendto(pctx->pcb, p, &pctx->addr);
		pbuf_free(p);
	}
}

enum retcode cmd_ping(struct context *ctx)
{
	err_t err=ERR_OK;
	struct ping_context *pctx=&cmd_ping_context;
	if (ctx->interrupt) {
		cmd_ping_exit(pctx, RET_SILENT);
	} else if (!ctx->poll) {
		pctx->ctx = ctx;
		pctx->pcb = raw_new(IP_PROTO_ICMP);
		// debug("raw_new %p %p\r\n",pctx, pctx->pcb);
		raw_recv(pctx->pcb, cmd_ping_recv, pctx);
		raw_bind(pctx->pcb, IP_ADDR_ANY);
		if (ipaddr_aton(ctx->s1, &pctx->addr))
			cmd_ping_send(pctx);
		else {
			err=dns_gethostbyname(ctx->s1, &pctx->addr, cmd_ping_found, pctx);
			if (err == ERR_INPROGRESS) {
				HMI_cprintf(ctx, "resolving\r\n");
				err = ERR_OK;
			}
		}
		if (err) {
			cmd_ping_exit(pctx,LWIP_ERR(err));
		} else
			pctx->done=RET_POLL_FAST;
	}
	return pctx->done;
}
