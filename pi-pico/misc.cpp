#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "../source/HMI_telnet.h"

void NVIC_SystemReset(void)
{ 
	watchdog_reboot(0, SRAM_END, 1);
}

void wait_ms(int ms)
{
	busy_wait_us(ms*1000LL);
}

void misc_loop(void)
{
	telnet_loop(NULL); 
	serial_term_loop();
}

void TDMA_init_all(void)
{
}

int SI4463_configure_all(void)
{
	return 1;
} 

void SI4463_clear_IT(SI4463_Chip* SI4463, unsigned char PH_clear, unsigned char modem_clear)
{
}

void SI4463_TX_to_RX_transition(void)
{
}

int SI4463_set_power(SI4463_Chip* SI4463)
{
	return 0;
}

void SI4432_TX_test(unsigned int req_duration)
{
}

void TDMA_NULL_frame_init(int size)
{
}

void call_bootloader(void)
{
	reset_usb_boot(0,0);
}
