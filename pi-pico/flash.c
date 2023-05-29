#include "pico_hal.h"
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"
#include "../source/HMI_telnet.h"
#include "npr70piextra.h"
#include "common.h"

extern const char* FS_BASE;
extern struct lfs_config pico_cfg;

static struct map {
	const void *source;
	int len;
} map[512];

static void __no_inline_not_in_flash_func(flash)(struct map *map, int count, int dst, int blocks)
{
#if 0
	rom_connect_internal_flash_fn connect_internal_flash = (rom_connect_internal_flash_fn)rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH);
	rom_flash_exit_xip_fn flash_exit_xip = (rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
	rom_flash_range_program_fn flash_range_program = (rom_flash_range_program_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_RANGE_PROGRAM);
	rom_flash_flush_cache_fn flash_flush_cache = (rom_flash_flush_cache_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);
#endif
	int idx=0;
	flash_range_erase(dst, blocks);

#if 0
    __compiler_memory_barrier();

    connect_internal_flash();
    flash_exit_xip();
    flash_range_program(flash_offs, data, count);
    flash_flush_cache(); // Note this is needed to remove CSn IO force as well as cache flushing
    flash_enable_xip_via_boot2();
#endif
#if 0
		flash_range_program(4096, data, 4);
#endif

#if 1
	while (idx < count) {
		flash_range_program(dst, map[idx].source, map[idx].len);
		dst+=map[idx].len;
		idx++;
		break;
	}
#endif
	for(;;);
	
}

int cmd_flash(struct context *ctx)
{
	int total_size,idx=0,fd=pico_open("npr70.bin",LFS_O_RDONLY);
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
		idx++;
                printf("%d %d %d %d\n",ret,block,offset, size);
                pos+=size;
        }

	pico_close(fd);
	save_and_disable_interrupts();
	flash(map, idx, 512*1024, (total_size+pico_cfg.block_size-1)/pico_cfg.block_size);
	return 3;
}
