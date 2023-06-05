#ifdef __cplusplus
extern "C" {
#endif
extern void debug(const char *str, ...);
extern void littlefs_init(void);
extern void bridge_setup(void);
extern void enchw_poll(void);
extern void enchw_init(void);
extern struct netif netif_data_eth;
#ifdef __cplusplus
}
#endif
