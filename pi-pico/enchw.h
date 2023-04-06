#include <stdint.h>
#define DEBUG debug
#define ENC28J60_USE_PBUF 1
typedef struct {
} enchw_device_t;
extern void enchw_setup(enchw_device_t *dev);
extern uint8_t enchw_exchangebyte(enchw_device_t *dev, uint8_t val);
extern void enchw_select(enchw_device_t *dev);
extern void enchw_unselect(enchw_device_t *dev);
