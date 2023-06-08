#ifdef __cplusplus
extern "C" {
#endif
extern void debug(const char *str, ...);
extern void littlefs_init(void);
extern void enchw_poll(void);
extern void enchw_init(void);
extern struct netif netif_eth;
extern struct netif netif_usb;
#ifdef __cplusplus
}
#endif
