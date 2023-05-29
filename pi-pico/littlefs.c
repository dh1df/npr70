#include "pico_hal.h"
#include "npr70piextra.h"
#include "common.h"

void littlefs_init(void)
{
	int ret=pico_mount(false);
	if (ret) {
		debug("mount returned %d, formatting\r\n",ret);
		ret=pico_mount(true);
		debug("mount returned %d\r\n",ret);
	} else {
		debug("mount successful\r\n");
	}
}

unsigned int virt_EEPROM_write(void *data, unsigned int previous_index)
{
	int ret,file=pico_open("config.bin",LFS_O_CREAT|LFS_O_RDWR);
	debug("virt_EEPROM_write file=%d\r\n",file);
	if (file >= 0) {
		ret=pico_rewind(file);
		debug("rewind %d\r\n",ret);
		ret=pico_write(file, data, 256);
		debug("write %d\r\n",ret);
		ret=pico_truncate(file, 256);
		debug("truncate %d\r\n",ret);
		ret=pico_fflush(file);
		debug("fflush %d\r\n",ret);
		ret=pico_close(file);
		debug("close %d\r\n",ret);
	}
	return previous_index+1;
}

void virt_EEPROM_errase_all(void)
{
	debug("virt_EEPROM_errase_all\r\n");
}

unsigned int virt_EEPROM_read(void *data)
{
	int ret,file=pico_open("config.bin",LFS_O_RDONLY);
	debug("virt_EEPROM_read file=%d\r\n",file);
	if (file >= 0) {
		ret=pico_read(file, data, 256);
		debug("ret %d\r\n",ret);
		if (ret == 256)
			return 1;
	}
	return 0;
}

int cmd_ls(struct context *ctx)
{
	return 3;
}

int cmd_rm(struct context *ctx)
{
	return 3;
}

int cmd_cat(struct context *ctx)
{
	return 3;
}

int cmd_wget(struct context *ctx)
{
	return 3;
}
