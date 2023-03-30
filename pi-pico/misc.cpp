#include "../source/HMI_telnet.h"

void NVIC_SystemReset(void) { };
void wait_ms(int ms) {};

void misc_loop(void)
{
	telnet_loop(NULL); 
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

unsigned int NFPR_config_save(void)
{
	return 0;
}

void virt_EEPROM_errase_all(void)
{
}

void DHCP_ARP_print_entries(void)
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
