#include "pico_hal.h"
#include "../source/HMI_telnet.h"
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

#ifndef HAVE_JSON_CONFIG
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
#else
void virt_EEPROM_errase_all(void)
{
	pico_remove("config.json");
}

#endif

int cmd_ls(struct context *ctx)
{
	struct lfs_info info;
	struct pico_fsstat_t fsstat;
	int dir=pico_dir_open("/");
	if (dir < 0) 
		return dir;
	while(pico_dir_read(dir, &info) > 0) {
		if (strcmp(info.name,".") && strcmp(info.name,".."))
			HMI_cprintf(ctx,"%s %d\r\n",info.name,info.size);
	}
	pico_dir_close(dir);
	pico_fsstat(&fsstat);
	HMI_cprintf(ctx,"%d*%d/%d(%d%%)\r\n",fsstat.block_size,fsstat.blocks_used,fsstat.block_count,fsstat.blocks_used*100/fsstat.block_count);
	return 3;
}

int cmd_rm(struct context *ctx)
{
	int err=pico_remove(ctx->s1);
	if (err < 0)
		return err;
	return 3;
}

int cmd_cat(struct context *ctx)
{
	char buffer[256];
	int file=pico_open(ctx->s1, LFS_O_RDONLY);
	if (file < 0)
		return file;
	for (;;) {
		int size=pico_read(file, buffer, sizeof(buffer));
		if (!size)
			break;
		HMI_cwrite(ctx, buffer, size);	
	}
	pico_close(file);
	return 3;
}

int cmd_cp(struct context *ctx)
{
	char buffer[256];
	int file1,file2;
	int ret=3;
	file1=pico_open(ctx->s1, LFS_O_RDONLY);
	if (file1 < 0)
		return file1;
	file2=pico_open(ctx->s2, LFS_O_CREAT|LFS_O_RDWR|LFS_O_TRUNC);
	if (file2 < 0)
		return file2;
	for (;;) {
		int size2,size1=pico_read(file1, buffer, sizeof(buffer));
		if (!size2)
			break;
		size2=pico_write(file2, buffer, size1);
		if (size2 != size1) {
			ret=LFS_ERR_NOSPC;
			break;
		}
	}
	pico_close(file2);
	pico_close(file1);
	return ret;
}
