#define HAVE_CALL_BOOTLOADER 1
#define HAVE_DISPLAY_NET 1
#define HAVE_CMD_TEST 1
#define HAVE_EXTERNAL_EEPROM_CONFIG 1
#define HAVE_CMD_FS 1
#define HAVE_CMD_FLASH 1
#define HAVE_INTEGRATED_SRAM 1
#define HAVE_MAIN_H 1
#define HAVE_NO_W5500 1
#define HAVE_NO_SNMP 1
#define HAVE_JSON_CONFIG 1

void debug(const char *str, ...);
struct context;
int cmd_display_net(struct context *c);
int cmd_test(struct context *c);
int cmd_ls(struct context *c);
int cmd_rm(struct context *c);
int cmd_cat(struct context *c);
int cmd_cp(struct context *c);
int cmd_wget(struct context *c);
int cmd_flash(struct context *c);
unsigned int virt_EEPROM_write(void *data, unsigned int previous_index);
void virt_EEPROM_errase_all(void);
unsigned int virt_EEPROM_read(void *data);
void misc_loop(void);
void call_bootloader(void);
void ext_SRAM_read2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size);
void ext_SRAM_write2(void* loc_SPI, unsigned char* loc_data, unsigned int address, int size);
