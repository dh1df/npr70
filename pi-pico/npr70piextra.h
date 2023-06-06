#define HAVE_CALL_BOOTLOADER 1
#define HAVE_DISPLAY_NET 1
#define HAVE_EXTERNAL_EEPROM_CONFIG 1
#define HAVE_INTEGRATED_SRAM 1
#define HAVE_MAIN_H 1
#define HAVE_NO_W5500 1
#define HAVE_NO_SNMP 1
#define HAVE_JSON_CONFIG 1

#define CMD_FS\
        {"ls",cmd_ls},\
        {"rm",cmd_rm},\
        {"cat",cmd_cat},\
        {"cp",cmd_cp},\
        {"sum",cmd_sum},

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

#define CUSTOM_COMMANDS CMD_WGET CMD_FS CMD_FLASH CMD_TEST CMD_XSET CMD_XDISPLAY
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
int cmd_wget(struct context *c);
int cmd_flash(struct context *c);
int cmd_xset(struct context *c);
int cmd_xdisplay(struct context *c);
unsigned int virt_EEPROM_write(void *data, unsigned int previous_index);
void virt_EEPROM_errase_all(void);
unsigned int virt_EEPROM_read(void *data);
void misc_loop(void);
void call_bootloader(void);
void ext_SRAM_read2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size);
void ext_SRAM_write2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size);
