#include "pico_hal.h"
#include "../source/HMI_telnet.h"
#include "npr70piextra.h"
#include "common.h"
#include "lwip/apps/http_client.h"
#include "lwip/altcp.h"

static httpc_state_t *state;
struct wget_context {
	int fd;
	int count;
} wget_context;

static err_t recv_fn(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
	struct wget_context *ctx=arg;
	lfs_size_t size=pico_write(ctx->fd, p->payload, p->len);
	if (size == p->len && p->len == p->tot_len) {
		HMI_cprintf(NULL,"*");
		if (ctx->count++ >= 70) {
			debug("\r\n");
			ctx->count=0;		
		}
	} else
		HMI_cprintf(NULL,"recv_fn2 %d %d %d\r\n",p->tot_len,p->len,size);
	altcp_recved(conn, p->tot_len);
	pbuf_free(p);
	return ERR_OK;
}

static err_t headers_done_fn(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
	debug("headers_done_fn\r\n");
	return ERR_OK;
}

void result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
	struct wget_context *ctx=arg;
	debug("\r\nresult_fn err=%d\r\n",err);
	pico_fflush(ctx->fd);
	pico_close(ctx->fd);
}


static httpc_connection_t settings;
int cmd_wget(struct context *ctx)
{
	const char *host,*path,*file,*prefix="http://";
	char hostbuf[128];
	int hostlen;
	settings.result_fn = result_fn;
	settings.headers_done_fn = headers_done_fn;
	if (strncmp(ctx->s1,prefix,strlen(prefix)))
		return  LFS_ERR_INVAL;
	host=ctx->s1+strlen(prefix);
	path=strchr(host,'/');
	if (!path)
		return LFS_ERR_INVAL;
	hostlen=path-host;
	if (hostlen >= sizeof(hostbuf))
		return LFS_ERR_NAMETOOLONG;
	strncpy(hostbuf, host, hostlen);
	hostbuf[hostlen]='\0';
	file=strrchr(ctx->s1,'/');
	if (!file || !strlen(file))
		return LFS_ERR_INVAL;
	wget_context.fd=pico_open(file,LFS_O_WRONLY|LFS_O_CREAT|LFS_O_TRUNC);
	if (wget_context.fd < 0)
		return wget_context.fd;
	wget_context.count=0;
	err_t err=httpc_get_file_dns(hostbuf,80,path,&settings,recv_fn,&wget_context,&state);
	if (err != ERR_OK)
		return -(err+32);
	return 3;
}
