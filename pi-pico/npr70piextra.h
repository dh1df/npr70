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

#define CUSTOM_COMMANDS CMD_WGET CMD_FS CMD_FLASH CMD_TEST CMD_XSET CMD_XDISPLAY CMD_BOOTLOADER CMD_PING CMD_TRACE
#define CUSTOM_DISPLAY_COMMANDS {"net",cmd_display_net},

void debug(const char *str, ...);
struct context;
int cmd_display_net(struct context *c);
int cmd_test(struct context *c);
int cmd_ls(struct context *c);
int cmd_rm(struct context *c);
int cmd_cat(struct context *c);
int cmd_cp(struct context *c);
int cmd_sum(struct context *c);
int cmd_mv(struct context *c);
int cmd_wget(struct context *c);
int cmd_flash(struct context *c);
int cmd_xset(struct context *c);
int cmd_xdisplay(struct context *c);
int cmd_bootloader(struct context *c);
int cmd_ping(struct context *c);
int cmd_trace(struct context *c);
int cmd_uptime(struct context *c);
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
