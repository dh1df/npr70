#include "pico_hal.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "../source/HMI_telnet.h"
#include "npr70piextra.h"
#include "common.h"

#define FLASH_BLOCK_ERASE_CMD 0xd8

extern const char* FS_BASE;
extern struct lfs_config pico_cfg;

static struct map {
	const unsigned char *source;
	int len;
} map[512];

volatile static int step;
static unsigned char flash_buffer[4096];
#define WATCHDOG_TIMEOUT 2000

static void __no_inline_not_in_flash_func(flash)(struct map *map, int count, int dst, int blocks, int block_size)
{
	int idx=0,pos=0;
	step=1;
	flash_range_erase(dst, blocks*block_size);
	watchdog_hw->load = WATCHDOG_TIMEOUT*1000*2;
	step=2;
	while (idx < count) {
		int i,len=map[idx].len;
		const unsigned char *source=map[idx].source;
		for (i = 0 ; i < len ; i++) {
			flash_buffer[pos++]=source[i];
			if (pos >= block_size) {
				watchdog_hw->load = WATCHDOG_TIMEOUT*1000*2;
				flash_range_program(dst, flash_buffer, pos);
				dst+=pos;
				pos=0;
			}
		}
		idx++;
	}
	while (pos < block_size)
		flash_buffer[pos++]=0xff;
	watchdog_hw->load = WATCHDOG_TIMEOUT*1000*2;
	flash_range_program(dst, flash_buffer, pos);
	step=3;
	for(;;);
}

int cmd_flash(struct context *ctx)
{
	int total_size,idx=0,fd=pico_open(ctx->s1,LFS_O_RDONLY);
	lfs_off_t pos=0;
	if (fd < 0)
		return fd;
	total_size=pico_size(fd);
        while (pos < total_size) {
                lfs_block_t block;
                lfs_off_t offset;
                int ret=lfs_bmap(NULL, (lfs_file_t*)fd, pos, &block, &offset);
		int size=4096-offset;
		if (size > total_size-pos)
			size=total_size-pos;
		map[idx].source=FS_BASE + XIP_NOCACHE_NOALLOC_BASE + (block * pico_cfg.block_size) + offset;
		map[idx].len=size;
                printf("%d %d %d %p %d\n",ret,block,offset, map[idx].source, size);
		idx++;
                pos+=size;
        }

	pico_close(fd);
	save_and_disable_interrupts();
	watchdog_reboot(0, SRAM_END, WATCHDOG_TIMEOUT);
	flash(map, idx, 0, (total_size+pico_cfg.block_size-1)/pico_cfg.block_size, pico_cfg.block_size);
	return 3;
}
