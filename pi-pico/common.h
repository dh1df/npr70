#ifdef __cplusplus
extern "C" {
#endif
#define INT_SRAM_SIZE (128*1024)
extern unsigned char int_sram[INT_SRAM_SIZE];

extern unsigned char CONF_ethernet_MAC[6];
extern unsigned char CONF_bridge_MAC[6];
extern void debug(const char *str, ...);
extern void littlefs_init(void);
extern void enchw_poll(void);
extern void enchw_init(void);
extern struct netif netif_eth;
extern struct netif netif_usb;
void trace_loop(void);
#ifdef __cplusplus
}
#endif
