#include "pico_hal.h"
#include "lwip/err.h"
#include "lwip/apps/http_client.h"
#include "../source/HMI_telnet_def.h"

#define HAVE_CALL_BOOTLOADER 1
#define HAVE_DISPLAY_NET 1
#define HAVE_EXTERNAL_EEPROM_CONFIG 1
#define HAVE_INTEGRATED_SRAM 1
#define HAVE_MAIN_H 1
#define HAVE_NO_W5500 1
#define HAVE_NO_SNMP 1
#define HAVE_JSON_CONFIG 1
#define CS_DELAY() wait_ns(1)

#define CMD_FS\
        {"ls",cmd_ls},\
        {"rm",cmd_rm},\
        {"cat",cmd_cat},\
        {"cp",cmd_cp},\
        {"sum",cmd_sum},\
	{"mv",cmd_mv},

#define CMD_WGET\
        {"wget",cmd_wget},

#define CMD_FLASH\
        {"flash",cmd_flash},

#define CMD_TEST\
        {"test",cmd_test},

#define CMD_XSET\
        {"xset",cmd_xset},

#define CMD_XDISPLAY\
        {"xdisplay",cmd_xdisplay},

#define CMD_BOOTLOADER\
        {"bootloader",cmd_bootloader},

#define CMD_PING\
        {"ping",cmd_ping},

#define CMD_TRACE\
        {"trace",cmd_trace},

#define CMD_UPTIME\
        {"uptime",cmd_uptime},

#define CUSTOM_COMMANDS CMD_WGET CMD_FS CMD_FLASH CMD_TEST CMD_XSET CMD_XDISPLAY CMD_BOOTLOADER CMD_PING CMD_TRACE CMD_UPTIME
#define CUSTOM_DISPLAY_COMMANDS {"net",cmd_display_net},


enum retcode;

void debug(const char *str, ...);
enum retcode cmd_display_net(struct context *c);
enum retcode cmd_test(struct context *c);
enum retcode cmd_ls(struct context *c);
enum retcode cmd_rm(struct context *c);
enum retcode cmd_cat(struct context *c);
enum retcode cmd_cp(struct context *c);
enum retcode cmd_sum(struct context *c);
enum retcode cmd_mv(struct context *c);
enum retcode cmd_wget(struct context *c);
enum retcode cmd_flash(struct context *c);
enum retcode cmd_xset(struct context *c);
enum retcode cmd_xdisplay(struct context *c);
enum retcode cmd_bootloader(struct context *c);
enum retcode cmd_ping(struct context *c);
enum retcode cmd_trace(struct context *c);
enum retcode cmd_uptime(struct context *c);
void virt_EEPROM_errase_all(void);
#if 0
unsigned int virt_EEPROM_write(void *data, unsigned int previous_index);
unsigned int virt_EEPROM_read(void *data);
#endif
void misc_loop(void);
void call_bootloader(void);
void ext_SRAM_read2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size);
void ext_SRAM_write2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size);

#define TRACE_RX_RADIO trace_rx_radio
#define TRACE_TX_RADIO trace_tx_radio
extern void trace_rx_radio(unsigned int us, int initial, unsigned char *data, int mask, int offset, int len);
extern void trace_tx_radio(unsigned int us, int initial, unsigned char *data, int len);

#define LWIP_ERR(x) ((enum retcode)(-((x)+30)))
#define LFS_ERR(x) ((enum retcode)(x))
#define HTTPC_ERR(x) ((enum retcode)(-((x)+50)))

#define CUSTOM_ERRORS_LFS \
	{LFS_ERR(LFS_ERR_IO), "Error during device operation"},\
	{LFS_ERR(LFS_ERR_CORRUPT), "Corrupted"},\
	{LFS_ERR(LFS_ERR_NOENT), "No directory entry"},\
	{LFS_ERR(LFS_ERR_EXIST), "Entry already exists"},\
	{LFS_ERR(LFS_ERR_NOTDIR), "Entry is not a dir"},\
	{LFS_ERR(LFS_ERR_ISDIR), "Entry is a dir"},\
	{LFS_ERR(LFS_ERR_NOTEMPTY), "Dir is not empty"},\
	{LFS_ERR(LFS_ERR_BADF), "Bad file number"},\
	{LFS_ERR(LFS_ERR_FBIG), "File too large"},\
	{LFS_ERR(LFS_ERR_INVAL), "Invalid parameter"},\
	{LFS_ERR(LFS_ERR_NOSPC), "No space left on device"},\
	{LFS_ERR(LFS_ERR_NOMEM), "No more memory available"},\
	{LFS_ERR(LFS_ERR_NOATTR), "No data/attr available"},\
	{LFS_ERR(LFS_ERR_NAMETOOLONG), "File name too long"},

#define CUSTOM_ERRORS_LWIP \
	{LWIP_ERR(ERR_MEM),"Out of memory error"},\
	{LWIP_ERR(ERR_BUF),"Buffer error"},\
	{LWIP_ERR(ERR_TIMEOUT),"Timeout"},\
	{LWIP_ERR(ERR_RTE),"Routing problem"},\
	{LWIP_ERR(ERR_INPROGRESS),"Operation in progress"},\
	{LWIP_ERR(ERR_VAL),"Illegal value"},\
	{LWIP_ERR(ERR_WOULDBLOCK), "Operation would block"},\
	{LWIP_ERR(ERR_USE),"Address in use"},\
	{LWIP_ERR(ERR_ALREADY),"Already connecting"},\
	{LWIP_ERR(ERR_ISCONN),"Conn already established"},\
	{LWIP_ERR(ERR_CONN),"Not connected"},\
	{LWIP_ERR(ERR_IF),"Low-level netif error"},\
	{LWIP_ERR(ERR_ABRT),"Connection aborted"},\
	{LWIP_ERR(ERR_RST),"Connection reset"},\
	{LWIP_ERR(ERR_CLSD),"Connection closed"},\
	{LWIP_ERR(ERR_ARG),"Illegal argument"},

#define CUSTOM_ERRORS_HTTPC \
	{HTTPC_ERR(HTTPC_RESULT_ERR_UNKNOWN),  "Unknown error"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_CONNECT),  "Connection to server failed"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_HOSTNAME),  "Failed to resolve server hostname"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_CLOSED),  "Connection unexpectedly closed by remote server"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_TIMEOUT),  "Connection timed out (server didn't respond in time)"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_SVR_RESP),  "Server responded with an error code"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_MEM),  "Local memory error"},\
	{HTTPC_ERR(HTTPC_RESULT_LOCAL_ABORT),  "Local abort"},\
	{HTTPC_ERR(HTTPC_RESULT_ERR_CONTENT_LEN),  "Content length mismatch"},

#define CUSTOM_ERRORS CUSTOM_ERRORS_LFS CUSTOM_ERRORS_LWIP CUSTOM_ERRORS_HTTPC
