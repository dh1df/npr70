// This file is part of "NPR70 modem firmware" software
// (A GMSK data modem for ham radio 430-440MHz, at several hundreds of kbps) 
// Copyright (c) 2017-2020 Guillaume F. F4HDK (amateur radio callsign)
// 
// "NPR70 modem firmware" is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// "NPR70 modem firmware" is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with "NPR70 modem firmware".  If not, see <http://www.gnu.org/licenses/>

#include "HMI_telnet.h"
#include "mbed.h"
#include "global_variables.h"
#include "Eth_IPv4.h"
#include "signaling.h"
#include "config_flash.h"
#include "W5500.h"
#include "SI4463.h"
#include "TDMA.h"
#include "DHCP_ARP.h"
#include "L1L2_radio.h"

static char current_rx_line[100];
//static char HMI_out_str[100];
static int current_rx_line_count = 0;
static int is_telnet_opened = 0;
static int echo_ON = 1; 

static unsigned int telnet_last_activity;
static struct context ctx;

static int HMI_cmd_display(struct context *c);
static int HMI_cmd_exit(struct context *c);
static int HMI_cmd_radio(struct context *c);
static int HMI_cmd_reboot(struct context *c);
static int HMI_cmd_reset_to_default(struct context *c);
static int HMI_cmd_save(struct context *c);
static int HMI_cmd_set(struct context *c);
static int HMI_cmd_status(struct context *c);
static int HMI_cmd_tx_test(struct context *c);
static int HMI_cmd_version(struct context *c);
static int HMI_cmd_who(struct context *c);

static int HMI_cmd_set_callsign(struct context *c);
static int HMI_cmd_set_client_req_size(struct context *c);
static int HMI_cmd_set_def_route_active(struct context *c);
static int HMI_cmd_set_def_route_val(struct context *c);
static int HMI_cmd_set_dhcp_active(struct context *c);
static int HMI_cmd_set_dns_active(struct context *c);
static int HMI_cmd_set_dns_value(struct context *c);
static int HMI_cmd_set_freq_shift(struct context *c);
static int HMI_cmd_set_frequency(struct context *c);
static int HMI_cmd_set_ip_beginn(struct context *c);
static int HMI_cmd_set_is_master(struct context *c);
static int HMI_cmd_set_master_down_ip(struct context *c);
static int HMI_cmd_set_master_fdd(struct context *c);
static int HMI_cmd_set_master_ip_size(struct context *c);
static int HMI_cmd_set_modem_ip(struct context *c);
static int HMI_cmd_set_modulation(struct context *c);
static int HMI_cmd_set_netmask(struct context *c);
static int HMI_cmd_set_radio_netw_id(struct context *c);
static int HMI_cmd_set_radio_on_at_startup(struct context *c);
static int HMI_cmd_set_rf_power(struct context *c);
static int HMI_cmd_set_telnet_active(struct context *c);
static int HMI_cmd_set_telnet_routed(struct context *c);

static int HMI_cmd_display_config(struct context *c);
static int HMI_cmd_display_dhcp_arp(struct context *c);
static int HMI_cmd_display_static(struct context *c);

static struct command commands[]={
#ifdef CUSTOM_COMMANDS
	CUSTOM_COMMANDS
#endif
	{"display", HMI_cmd_display},
	{"exit", HMI_cmd_exit},
	{"quit", HMI_cmd_exit},
	{"radio", HMI_cmd_radio},
	{"reboot", HMI_cmd_reboot},
	{"reset_to_default", HMI_cmd_reset_to_default},
	{"save", HMI_cmd_save},
	{"set", HMI_cmd_set},
	{"status", HMI_cmd_status},
	{"TX_test", HMI_cmd_tx_test},
	{"version", HMI_cmd_version},
	{"who", HMI_cmd_who},
};

static struct command display_commands[]={
#ifdef CUSTOM_DISPLAY_COMMANDS
	CUSTOM_DISPLAY_COMMANDS
#endif
	{"config", HMI_cmd_display_config},
	{"DHCP_ARP", HMI_cmd_display_dhcp_arp},
	{"static", HMI_cmd_display_static},
};

static struct command set_commands[]={
	{"callsign", HMI_cmd_set_callsign},
	{"client_req_size", HMI_cmd_set_client_req_size},
	{"def_route_active", HMI_cmd_set_def_route_active},
	{"def_route_val", HMI_cmd_set_def_route_val},
	{"DHCP_active", HMI_cmd_set_dhcp_active},
	{"dns_active", HMI_cmd_set_dns_active},
	{"DNS_value", HMI_cmd_set_dns_value},
	{"freq_shift", HMI_cmd_set_freq_shift},
	{"frequency", HMI_cmd_set_frequency},
	{"IP_begin", HMI_cmd_set_ip_beginn},
	{"is_master", HMI_cmd_set_is_master},
	{"master_down_IP", HMI_cmd_set_master_down_ip},
	{"master_FDD", HMI_cmd_set_master_fdd},
	{"master_IP_size", HMI_cmd_set_master_ip_size},
	{"modem_IP", HMI_cmd_set_modem_ip},
	{"modulation", HMI_cmd_set_modulation},
	{"netmask", HMI_cmd_set_netmask},
	{"radio_netw_ID", HMI_cmd_set_radio_netw_id},
	{"radio_on_at_start", HMI_cmd_set_radio_on_at_startup},
	{"RF_power", HMI_cmd_set_rf_power},
	{"telnet_active", HMI_cmd_set_telnet_active},
	{"telnet_routed", HMI_cmd_set_telnet_routed},
};

static int HMI_command_help(struct context *ctx, struct command *cmd, int len, int hor)
{
	int i,col;
	for (i = 0 ; i < len ; i++) {
		if (hor) {
			HMI_cprintf(ctx, " %s",cmd->cmd);
			col+=strlen(cmd->cmd)+1;
			if (col > 60) {
				HMI_cprintf(ctx, "\r\n");
				col=0;
			}
		} else
			HMI_cprintf(ctx,"%s\r\n",cmd->cmd);
		cmd++;
	}
	if (hor && col)
		HMI_cprintf(ctx, "\r\n");
	return 2;
}

int HMI_command_parse(struct context *ctx, const char *s, struct command *cmd, int len, int help)
{
	int i;
	if (help && !strcmp(s,"help"))
		return HMI_command_help(ctx, cmd, len,0 );
	for (i = 0 ; i < len ; i++) {
		if (!strcmp(cmd->cmd, s))
			return cmd->func(ctx);
		cmd++;
	}
	return 0;
}

void
HMI_prompt(struct context *c)
{
	HMI_cprintf(c,"ready> ");
}

void
HMI_prompt_ctrlc(struct context *c)
{
	HMI_printf("CTRL+c to exit...\r\n");
}

static void
HMI_enable_echo(void)
{
	echo_ON = 1;
}

/**
 * Called regularly by the main loop, and manages network events (new connection,
 * data, etc)
 */
int telnet_loop (W5500_chip* W5500) {
	static unsigned char previous_state = 0;
	uint8_t RX_data[100];
	uint8_t TX_data[100];
	uint8_t current_state; // Socket state as returned by the W5500
	unsigned int timer_snapshot;
	char loc_char;
	int RX_size = 0;
	int i, j;
	int result;

	result=0;
	
	current_state = W5500_read_byte(W5500, W5500_Sn_SR, TELNET_SOCKET);
	//printf("state: %x\r\n", current_state);
	if ((current_state == W5500_SOCK_ESTABLISHED) && (previous_state != W5500_SOCK_ESTABLISHED)) {
		W5500_read_long(W5500, W5500_Sn_DIPR0, TELNET_SOCKET, RX_data, 7);
		// Note: RX_data starts with 3 extra bytes (W5500 register that was read) that we don't want
		printf("\r\n\r\nnew telnet connexion from %i.%i.%i.%i\r\nserial inactive...\r\n", RX_data[3], RX_data[4], RX_data[5], RX_data[6]);
		fflush(stdout);
		//TX_data[0] = 0xFF; //IAC
		//TX_data[1] = 0xFB; //WILL FB DO FD
		//TX_data[2] = 1; //echo 1 RTCE 7
		//TX_data[3] = 0;
		TX_data[0] = 0xFF; //IAC
		TX_data[1] = 0xFB; //WILL 
		TX_data[2] = 0x01; //Echo
		//TX_data[3] = 0;
		TX_data[3] = 0xFF; //IAC
		TX_data[4] = 0xFD; //DO
		TX_data[5] = 0x03; //Suppr GA 
		TX_data[6] = 0xFF; //IAC
		TX_data[7] = 0xFB; //WILL
		TX_data[8] = 0x03; //Suppr GA 
		TX_data[9] = 0;
		strcat((char*)TX_data, "NPR modem\r\n");
		W5500_write_TX_buffer (W5500, TELNET_SOCKET, TX_data, strlen((char *)TX_data), 0); //27
		//HMI_printf("ready>");
		is_telnet_opened = 1;
		current_rx_line_count = 0;
		HMI_enable_echo();
		HMI_prompt(NULL);
		telnet_last_activity = GLOBAL_timer.read_us();
	}
	
	if (current_state == W5500_SOCK_WAIT) { // close wait to close 
		W5500_write_byte(W5500, W5500_Sn_CR, TELNET_SOCKET, 0x10); 
		printf("telnet connexion closed\r\n");
		fflush(stdout);
		is_telnet_opened = 0;
		current_rx_line_count = 0;
		ctx.ret = 0;
		HMI_enable_echo();
		HMI_prompt(NULL);
	}
	
	if (current_state == W5500_SOCK_CLOSED) { //closed to open
		W5500_write_byte(W5500, W5500_Sn_CR, TELNET_SOCKET, 0x01); 
		//printf("open \r\n");
		result=1;
	}
	
	if (current_state == W5500_SOCK_INIT) { //opened to listen
		W5500_write_byte(W5500, W5500_Sn_CR, TELNET_SOCKET, 0x02); 
		//printf("listen \r\n");
	}
	previous_state = current_state;
	if (is_telnet_opened) {
		RX_size = W5500_read_received_size(W5500, TELNET_SOCKET); 
		timer_snapshot = GLOBAL_timer.read_us();
		if ((timer_snapshot - telnet_last_activity) > 300000000) { //300000000
			//HMI_printf("Telnet inactivity timeout. Force exit.\r\n");
			W5500_write_byte(W5500_p1, W5500_Sn_CR, TELNET_SOCKET, 0x08); //close TCP
			is_telnet_opened = 0;
			ctx.ret = 0;
			HMI_enable_echo();
			printf("telnet connexion closed\r\n"); 
			fflush(stdout);
			HMI_prompt(NULL);
		}
		//timeout 
	}
	if (RX_size > 0) {
		telnet_last_activity = GLOBAL_timer.read_us();
		result=1;
		// printf("RX Size: %i\r\n", RX_size);
		W5500_read_RX_buffer(W5500, TELNET_SOCKET, RX_data, RX_size+3);
		// Note: RX_data starts with three extra bytes (W5500 register/block info) that are not
		// part of the actual received data
		RX_data[RX_size+3] = 0;
		i = 3;
		j = 0;
		while (i < (RX_size+3)) {
			loc_char = (char)RX_data[i];
			// printf("%02X %c\r\n", loc_char, loc_char);
			if ( (loc_char >= 0x20) && (loc_char <= 0x7E) ) {//displayable char
				if ( (current_rx_line_count < 98) && (echo_ON) ) {
					TX_data[j]=RX_data[i];
					i++;
					j++;
					current_rx_line[current_rx_line_count] = loc_char;
					current_rx_line_count++;
				} else {
					i++;
				}
			} 
			else { // special char
				if (loc_char == 0xFF) {//IAC
					if (RX_data[i+1] == 244) {//ctrl+C
						HMI_cancel_current(&ctx);
					}
					i = i + 3;
				}
				else if ( ( (loc_char == 0x08) || (loc_char == 0x7F) ) && (echo_ON) ) { //backspace
					i++;
					
					if (current_rx_line_count>0) {
						current_rx_line_count--;
						TX_data[j] = 0x08;
						TX_data[j+1] = 0x20;
						TX_data[j+2] = 0x08;
						j=j+3;
					}
				}
				else if ( (loc_char == 0x0D) && (echo_ON) ){ //end of line
					TX_data[j] = 0x0D;
					TX_data[j+1] = 0x0A;
					i++;
					j = j + 2;
					current_rx_line[current_rx_line_count] = 0;//null termination
					current_rx_line_count++;
					W5500_write_TX_buffer (W5500, TELNET_SOCKET, TX_data, j, 0);
					j = 0;
					HMI_line_parse (&ctx, current_rx_line, current_rx_line_count);
					current_rx_line_count = 0;
				}
				else if (loc_char == 0x03) { //ctrl + C
					HMI_cancel_current(&ctx);
					//printf("CTRL + C\r\n");
					i++;
				} else {
					i++;
				}
			}
		}
		if (j > 0) {
			W5500_write_TX_buffer (W5500, TELNET_SOCKET, TX_data, j, 0);
		}
		//printf("\r\n");
	}
	return result;
}

int serial_term_loop (void) {
	char loc_char;
	
	if (pc.readable()) {
		loc_char = getc(pc);
		
		if (is_telnet_opened == 0) {
			if ( (loc_char >= 0x20) && (loc_char <= 0x7E) ) {//printable char
				if ( (current_rx_line_count < 98) && (echo_ON) ) {
					putc(loc_char, pc);
					current_rx_line[current_rx_line_count] = loc_char;
					current_rx_line_count++;
				}
			}
			else {
				if ( ( (loc_char == 0x08) || (loc_char == 0x7F) ) && (echo_ON) ) {//backspace
					if (current_rx_line_count>0) {
						current_rx_line_count--;
						putc(0x08,pc);
						putc(0x20,pc);
						putc(0x08,pc);
					}
				}
				else if ( (loc_char == 0x0D) && (echo_ON) ) {
					printf("\r\n");
					current_rx_line[current_rx_line_count] = 0;
					current_rx_line_count++;
					HMI_line_parse (&ctx, current_rx_line, current_rx_line_count);
					current_rx_line_count = 0;
				}
				else if (loc_char == 0x03) {//ctrl + c
					HMI_cancel_current(&ctx);
				}
			}
		}
		return 1;
	} else {
		return 0;
	}
}

static int HMI_cmd_radio(struct context *c)
{
	if (strcmp(c->s1, "on") == 0) {
		if (CONF_radio_state_ON_OFF == 0) {
			RADIO_on(1, 1, 1);
		}
	}
	else if (strcmp(c->s1, "off") == 0) {
		RADIO_off(1);
	}
	else {
		HMI_printf("unknown radio command, use on or off\r\n");
		return 2;
	}
	return 3;
}

static int HMI_cmd_display(struct context *c)
{
	int command_understood = HMI_command_parse(c, c->s1, display_commands, sizeof(display_commands)/sizeof(display_commands[0]), 1);
	if (command_understood)
		return command_understood;
	HMI_printf("unknown display command, use display help for help\r\n");
	return 2;
}

static int HMI_cmd_version(struct context *c)
{
	HMI_cprintf(c,"firmware: %s\r\nfreq band: %s\r\n", FW_VERSION, FREQ_BAND);
	return 1;
}

static int HMI_cmd_reset_to_default(struct context *c)
{
	HMI_printf("clearing saved config...\r\n");
	RADIO_off_if_necessary(0);
	virt_EEPROM_errase_all();
	HMI_printf("Done. Now rebooting...\r\n");
	NVIC_SystemReset();
	return 1;
}

static int HMI_cmd_save(struct context *c)
{
	int temp;
	RADIO_off_if_necessary(0);
	temp = NFPR_config_save();
	RADIO_restart_if_necessary(0, 0, 1);
	HMI_printf("saved index:%i\r\n", temp);
	return 2;
}

int HMI_exec(struct context *c)
{
	char *loc_command_str = c->cmd;
	int command_understood=HMI_command_parse(&ctx, loc_command_str, commands, sizeof(commands)/sizeof(commands[0]), 1);
	c->ret = command_understood;
	if (command_understood == 4 || command_understood == 5) { /* 4=Call again slow, 5=Call again fast */
		echo_ON = 0;
		return command_understood;
	}
	if (command_understood == 3) { /* 3=Understood with OK and prompt */
		HMI_cprintf(c, "OK\r\n");
	}
	if (command_understood == 0) { /* 0=Not undestood */
		HMI_cprintf(c, "unknown command, use help for help\r\n");
	}
	if (command_understood < 0) { /* < 0=Error */
		HMI_cprintf(c, "ERR %d\r\n",command_understood);
	}
	if (command_understood >= 2 || command_understood <= 0) { /* 2=Understood with prompt */
		HMI_prompt(c);
	}
	/* 1=Understood */
	return command_understood;
}

void HMI_line_parse (struct context *c, char* RX_text, int RX_text_count) {
	c->cmd = strtok (RX_text, " ");
	c->s1 = strtok (NULL, " ");
	c->s2 = strtok (NULL, "\r");
	c->poll = 0;
	c->interrupt = 0;
	c->slow_counter = 0;

	if (c->cmd) {
		HMI_exec(c);
	} else {//just a return with nothing
		HMI_prompt(c);
	}
}

void HMI_cancel_current(struct context *c) {
	if (echo_ON ==0) {
		c->interrupt=1;
		c->ret=0;
		HMI_exec(c);
		HMI_enable_echo();
		HMI_prompt(&ctx);
	}
}

int HMI_check_radio_OFF(struct context *c) {
	if (CONF_radio_state_ON_OFF == 1) {
		HMI_cprintf(c, "radio must be off for this command\r\n");
		return 0;
	} else {
		return 1;
	}
}


int HMI_cmd_tx_test(struct context *c) {
	unsigned int duration;
	if ( HMI_check_radio_OFF(c) == 1) {
		sscanf (c->s1, "%i", &duration);
		HMI_cprintf(c, "reconfiguring radio...\r\n");
		SI4463_configure_all();
		wait_ms(50);
		TDMA_init_all();
		//SI4463_radio_start();
		
		wait_ms(1);
		G_SI4463->RX_TX_state = 0;
		SI4463_clear_IT(G_SI4463, 0, 0);
		wait_ms(10);
		CONF_radio_state_ON_OFF = 1;
		SI4463_TX_to_RX_transition();
		
		wait_ms(10);
		CONF_radio_state_ON_OFF = 0;
		if (!is_TDMA_master) {
			my_client_radio_connexion_state = 1;
			my_radio_client_ID = 0x7E;
		}
		wait_ms(50);
		
		TDMA_NULL_frame_init(70);
		HMI_printf("radio transmit test %i seconds...\r\n", duration);
		duration = duration * 1000; //milliseconds instead of seconds
		
		SI4432_TX_test(duration);
	
		return 3;	
	}
	return 2;
}

void HMI_close_telnet(void)
{
	if (is_telnet_opened == 1) {
		W5500_write_byte(W5500_p1, 0x0001, TELNET_SOCKET, 0x08);
	}
}


int HMI_cmd_reboot(struct context *c) {
	HMI_close_telnet();
	//extern "C" void mbed_reset();
	NVIC_SystemReset();
	return 2;
}

void HMI_force_exit(void) {
	unsigned char IP_loc[8];
	if (is_telnet_opened == 1) {
		IP_int2char (LAN_conf_applied.LAN_modem_IP, IP_loc);
		//HMI_printf("\r\n\r\n\r\nNew IP config... force telnet exit.\r\n");
		//HMI_printf("\r\n\r\nNew IP config. Open new telnet session with: %i.%i.%i.%i\r\n\r\n", IP_loc[0], IP_loc[1], IP_loc[2], IP_loc[3]);
		W5500_write_byte(W5500_p1, 0x0001, TELNET_SOCKET, 0x08); //close TCP
		is_telnet_opened = 0;
		ctx.ret = 0;
		printf("telnet connexion closed\r\n"); 
		fflush(stdout);
		HMI_enable_echo();
		HMI_prompt(NULL);
	}
}

int HMI_cmd_exit(struct context *c) {
	if (is_telnet_opened == 1) {
		W5500_write_byte(W5500_p1, 0x0001, TELNET_SOCKET, 0x08); //close TCP
		is_telnet_opened = 0;
		ctx.ret = 0;
		printf("telnet connexion closed\r\n"); 
		fflush(stdout);
		HMI_enable_echo();
		HMI_prompt(NULL);
	} else {
		printf("exit only valid for telnet\r\n");
		fflush(stdout);
		HMI_prompt(NULL);
	}
	return 1;
}

static char HMI_yes_no[2][4]={'n','o',0,0, 'y','e','s',0};
// static char HMI_trans_modes[2][4]={'I','P',0,0,'E','t','h',0};
static char HMI_master_FDD[3][5]={'n','o',0,0,0,'d','o','w','n',0,'u','p',0,0,0};

int HMI_cmd_display_config(struct context *c) {
	unsigned char IP_loc[8];

	HMI_printf("CONFIG:\r\n  callsign: '%s'\r\n  is_master: %s\r\n  MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", CONF_radio_my_callsign+2, HMI_yes_no[is_TDMA_master],CONF_modem_MAC[0],CONF_modem_MAC[1],CONF_modem_MAC[2],CONF_modem_MAC[3],CONF_modem_MAC[4],CONF_modem_MAC[5]);
	HMI_printf("  ext_SRAM: %s\r\n", HMI_yes_no[is_SRAM_ext]);
	HMI_printf("  frequency: %.3fMHz\r\n  freq_shift: %.3fMHz\r\n  RF_power: %i\r\n  modulation: %i\r\n", ((float)CONF_frequency_HD/1000)+FREQ_RANGE_MIN, (float)CONF_freq_shift/1000, CONF_radio_PA_PWR, CONF_radio_modulation); 

	HMI_printf("  radio_netw_ID: %i\r\n  radio_on_at_start: %s\r\n", CONF_radio_network_ID, HMI_yes_no[CONF_radio_default_state_ON_OFF]);
	HMI_printf("  telnet active: %s\r\n  telnet routed: %s\r\n", HMI_yes_no[is_telnet_active], HMI_yes_no[is_telnet_routed]);
	IP_int2char (LAN_conf_saved.LAN_modem_IP, IP_loc);
	IP_int2char (LAN_conf_saved.LAN_subnet_mask, IP_loc+4);
	HMI_printf("  modem_IP: %i.%i.%i.%i\r\n  netmask: %i.%i.%i.%i\r\n", IP_loc[0], IP_loc[1],IP_loc[2],IP_loc[3],IP_loc[4],IP_loc[5],IP_loc[6],IP_loc[7]);
	
	if (is_TDMA_master) {
		HMI_printf("  master_FDD: %s\r\n", HMI_master_FDD[CONF_master_FDD]);
	}
	
	
	if ( (is_TDMA_master) && ( CONF_master_FDD < 2 ) && (CONF_transmission_method==0) ) {//Master FDD down (or no FDD)	
		IP_int2char (CONF_radio_IP_start, IP_loc);
		IP_int2char (CONF_radio_IP_start+CONF_radio_IP_size-1, IP_loc+4);
		HMI_printf("  IP_begin: %i.%i.%i.%i\r\n  master_IP_size: %ld (Last IP: %i.%i.%i.%i)\r\n", IP_loc[0], IP_loc[1],IP_loc[2],IP_loc[3],CONF_radio_IP_size, IP_loc[4],IP_loc[5],IP_loc[6],IP_loc[7]);
		IP_int2char (LAN_conf_saved.LAN_def_route, IP_loc);	
		HMI_printf("  def_route_active: %s\r\n  def_route_val: %i.%i.%i.%i\r\n", HMI_yes_no[LAN_conf_saved.LAN_def_route_activ], IP_loc[0],IP_loc[1],IP_loc[2],IP_loc[3]);
		IP_int2char (LAN_conf_saved.LAN_DNS_value, IP_loc);
		HMI_printf("  DNS_active: %s\r\n  DNS_value: %i.%i.%i.%i\r\n", HMI_yes_no[LAN_conf_saved.LAN_DNS_activ], IP_loc[0],IP_loc[1],IP_loc[2],IP_loc[3]);
	}
	if ( (is_TDMA_master) && (CONF_master_FDD == 2) ) {//Master FDD up
		IP_int2char (CONF_master_down_IP, IP_loc);
		HMI_printf("  master_down_IP: %i.%i.%i.%i\r\n",IP_loc[0],IP_loc[1],IP_loc[2],IP_loc[3]);
	}
	if (!is_TDMA_master) {//client
		IP_int2char (CONF_radio_IP_start, IP_loc);
		HMI_printf("  IP_begin: %i.%i.%i.%i\r\n", IP_loc[0], IP_loc[1],IP_loc[2],IP_loc[3]);
		HMI_printf("  client_req_size: %ld\r\n  DHCP_active: %s\r\n", CONF_radio_IP_size_requested, HMI_yes_no[LAN_conf_saved.DHCP_server_active]);
	}
	return 2;
}

int HMI_cmd_display_dhcp_arp(struct context *c) {
	DHCP_ARP_print_entries();
	return 2;
}

int HMI_cmd_display_static(struct context *c) {
	return 2;
}

static int HMI_cmd_set_callsign(struct context *c)
{
	RADIO_off_if_necessary(1);
	strcpy (CONF_radio_my_callsign+2, c->s2);
	CONF_radio_my_callsign[0] = CONF_modem_MAC[4];
	CONF_radio_my_callsign[1] = CONF_modem_MAC[5];
	CONF_radio_my_callsign[15] = 0;
	RADIO_restart_if_necessary(1, 0, 1);
	HMI_cprintf(c, "new callsign '%s'\r\n", CONF_radio_my_callsign+2);
	return 3;
}

static int HMI_cmd_set_is_master(struct context *c)
{
	char DHCP_warning[50];
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		RADIO_off_if_necessary(1);
		is_TDMA_master = (temp_uchar == 1);
		RADIO_restart_if_necessary(1, 0, 1);
		if ( (is_TDMA_master) && (LAN_conf_saved.DHCP_server_active == 1) ) {
			strcpy (DHCP_warning, " (warning, DHCP inhibited in master mode)"); 
		} else {
			strcpy (DHCP_warning, ""); 
		}
		HMI_cprintf(c, "Master '%s'%s\r\n", c->s2, DHCP_warning);
	}
	return 3;
}

static int HMI_cmd_set_telnet_active(struct context *c)
{
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		if(is_telnet_opened) { HMI_cmd_exit(NULL); }
		is_telnet_active = temp_uchar;
		HMI_printf("telnet active '%s'\r\n", c->s2);
	}
	return 3;
}

static int HMI_cmd_set_telnet_routed(struct context *c)
{
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		is_telnet_routed = temp_uchar;
		//W5500_re_configure_gateway(W5500_p1);
		HMI_printf("telnet routed '%s'\r\n", c->s2);
	}
	return 3;
}

static int HMI_cmd_set_dns_active(struct context *c)
{
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		RADIO_off_if_necessary(1);
		LAN_conf_saved.LAN_DNS_activ = temp_uchar;
		RADIO_restart_if_necessary(1, 0, 1);
		HMI_printf("DNS active '%s'", c->s2);
	}
	return 3;
}

static int HMI_cmd_set_def_route_active(struct context *c)
{
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		RADIO_off_if_necessary(1);
		LAN_conf_saved.LAN_def_route_activ = temp_uchar;
		//W5500_re_configure_gateway(W5500_p1);
		RADIO_restart_if_necessary(1, 0, 1);
		HMI_printf("default route active '%s'\r\n", c->s2);
	}
	return 3;
}

static int HMI_cmd_set_master_fdd(struct context *c)
{
	if(strcmp(c->s2,"no") == 0) {
		CONF_master_FDD = 0;
		RADIO_off_if_necessary(1);
		RADIO_restart_if_necessary(1, 0, 1);
	}
	else if(strcmp(c->s2,"down") == 0) {
		CONF_master_FDD = 1;
		RADIO_off_if_necessary(1);
		RADIO_restart_if_necessary(1, 0, 1);
	}
	else if(strcmp(c->s2,"up") == 0) {
		CONF_master_FDD = 2;
		RADIO_off_if_necessary(1);
		RADIO_restart_if_necessary(1, 0, 1);
	}
	else {
		HMI_printf("  wrong value. Use no,down or up\r\n");
	}
	return 3;
}

static int HMI_cmd_set_radio_on_at_startup(struct context *c)
{
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		CONF_radio_default_state_ON_OFF = temp_uchar;
		HMI_printf("radio_on_at_start '%s'\r\n", c->s2);
	}
	return 3;
}

static int HMI_cmd_set_dhcp_active(struct context *c)
{
	char DHCP_warning[50];
	unsigned char temp_uchar = HMI_yes_no_2int(c->s2);
	if ( (temp_uchar==0) || (temp_uchar==1) ) {
		LAN_conf_saved.DHCP_server_active = temp_uchar;
		if ( (is_TDMA_master) && (LAN_conf_saved.DHCP_server_active == 1) ) {
			strcpy (DHCP_warning, " (warning, DHCP inhibited in master mode)"); 
		} else {
			strcpy (DHCP_warning, ""); 
		}
		HMI_printf("DHCP_active: '%s'%s\r\n", c->s2, DHCP_warning);
		return 3;
	}
	return 2;
}

static int HMI_cmd_set_modem_ip(struct context *c)
{
	unsigned long int temp_uint = HMI_str2IP(c->s2);
	if (temp_uint !=0) {
		RADIO_off_if_necessary(1);
		LAN_conf_saved.LAN_modem_IP = temp_uint;
		//HMI_force_exit();
		//W5500_re_configure();
		RADIO_restart_if_necessary(1, 0, 1);
	}
	return 3;
}

static int HMI_cmd_set_netmask(struct context *c)
{
	unsigned long int temp_uint = HMI_str2IP(c->s2);
	if (temp_uint !=0) {
		RADIO_off_if_necessary(1);
		return 3;
		LAN_conf_saved.LAN_subnet_mask = temp_uint;
		//HMI_force_exit();
		//W5500_re_configure();
		RADIO_restart_if_necessary(1, 0, 1);
	}
	return 3;
}

static int HMI_cmd_set_def_route_val(struct context *c)
{
	unsigned long int temp_uint = HMI_str2IP(c->s2);
	if (temp_uint !=0) {
		RADIO_off_if_necessary(1);
		LAN_conf_saved.LAN_def_route = temp_uint;
		//W5500_re_configure_gateway(W5500_p1);
		RADIO_restart_if_necessary(1, 0, 1);
	}
	return 3;
}

static int HMI_cmd_set_dns_value(struct context *c)
{
	unsigned long int temp_uint = HMI_str2IP(c->s2);
	if (temp_uint !=0) {
		RADIO_off_if_necessary(1);
		LAN_conf_saved.LAN_DNS_value = temp_uint;
		RADIO_restart_if_necessary(1, 0, 1);
	}
	return 3;
}


static int HMI_cmd_set_ip_beginn(struct context *c)
{
	unsigned long int temp_uint = HMI_str2IP(c->s2);
	if (temp_uint !=0) {
		RADIO_off_if_necessary(1);
		CONF_radio_IP_start = temp_uint;
		RADIO_restart_if_necessary(1, 0, 1);
	}
	return 3;
}

static int HMI_cmd_set_master_down_ip(struct context *c)
{
	unsigned long int temp_uint = HMI_str2IP(c->s2);
	if (temp_uint !=0) {
		RADIO_off_if_necessary(1);
		CONF_master_down_IP = temp_uint;
		RADIO_restart_if_necessary(1, 0, 1);
	}
	return 3;
}

static int HMI_cmd_set_master_ip_size(struct context *c)
{
	unsigned long int temp_uint;
	int temp = sscanf (c->s2, "%ld", &temp_uint);
	if ( (temp==1) && (temp_uint!=0) ) {
		RADIO_off_if_necessary(1);
		CONF_radio_IP_size = temp_uint;
		RADIO_restart_if_necessary(1, 0, 1);
		return 3;
	}
	else {
		HMI_printf("wrong value\r\n");
	}
	return 2;
}

static int HMI_cmd_set_client_req_size(struct context *c)
{
	unsigned long int temp_uint;
	int temp = sscanf (c->s2, "%ld", &temp_uint);
	if ( (temp==1) && (temp_uint!=0) ) {
		RADIO_off_if_necessary(1);
		CONF_radio_IP_size_requested = temp_uint;
		RADIO_restart_if_necessary(1, 0, 1);
		return 3;
	}
	else {
		HMI_printf("wrong value\r\n");
	}
	return 2;
}


static int HMI_cmd_set_frequency(struct context *c)
{
	float frequency;
	int temp = sscanf (c->s2, "%f", &frequency);
	if ( (temp == 1) && (frequency<=FREQ_RANGE_MAX) && (frequency>FREQ_RANGE_MIN) ) {
		RADIO_off_if_necessary(0);
		frequency = (frequency - FREQ_RANGE_MIN)*1000 + 0.3; 
		CONF_frequency_HD = (short int)frequency;
		//RADIO_compute_freq_params();//REMOVE TEST
		RADIO_restart_if_necessary(0, 1, 1);
		return 3;
	} else {
		HMI_printf("wrong freq value\r\n");
	}
	return 2;
}

static int HMI_cmd_set_freq_shift(struct context *c)
{
	float frequency;
	int temp = sscanf (c->s2, "%f", &frequency);
	if ( (temp == 1) && (frequency<=10) && (frequency>=-10) ) {
		RADIO_off_if_necessary(0);
		frequency = (frequency*1000);
		CONF_freq_shift = (short int)frequency;
		//RADIO_compute_freq_params();//REMOVE TEST
		//if (CONF_frequency_band == previous_freq_band) {
		//	RADIO_restart_if_necessary(0, 0, 1);
		//}else {
		RADIO_restart_if_necessary(0, 1, 1);
		//}
		return 3;
	} else {
		HMI_printf("wrong freq value\r\n");
	}
	return 2;
}

static int HMI_cmd_set_rf_power(struct context *c)
{
	int temp;
	unsigned long int temp_uint = sscanf (c->s2, "%i", &temp); 
	if ( (temp_uint == 1) && (temp<128) ) {
		RADIO_off_if_necessary(0);
		CONF_radio_PA_PWR = temp;
		SI4463_set_power(G_SI4463);
		RADIO_restart_if_necessary(0, 0, 1);
		return 3;
	} else {
		HMI_printf("error : max RF_power value 127");
	}
	return 2;
}


static int HMI_cmd_set_modulation(struct context *c)
{
	int temp;
	unsigned long int temp_uint = sscanf (c->s2, "%i", &temp);
	unsigned char temp_uchar = temp;
	//if ( (temp_uint == 1) && ((temp_uchar==13)||(temp_uchar==14)||(temp_uchar==22)||(temp_uchar==23)||(temp_uchar==24)) ) {
	if ( (temp_uint == 1) && ( ((temp_uchar>=11)&&(temp_uchar<=14)) || ((temp_uchar>=20)&&(temp_uchar<=24)) ) ) {
		RADIO_off_if_necessary(1);
		CONF_radio_modulation = temp_uchar;
		RADIO_restart_if_necessary(1, 1, 1);
		return 3;
	} else {
		HMI_printf("wrong modulation value");
	}
	return 2;
}

static int HMI_cmd_set_radio_netw_id(struct context *c)
{
	int temp;
	unsigned long int temp_uint = sscanf (c->s2, "%i", &temp);
	unsigned char temp_uchar = temp;
	if ( (temp_uint == 1) && (temp_uchar <= 15) ) {
		RADIO_off_if_necessary(1);
		CONF_radio_network_ID = temp_uchar;
		RADIO_restart_if_necessary(1, 1, 1);
		return 3;
	} else {
		HMI_printf("wrong value, 15 max");
	}
	return 2;
}

static int HMI_cmd_set(struct context *c) {
	char* loc_param1=c->s1;
	char* loc_param2=c->s2;
	// unsigned char previous_freq_band;
	if ((loc_param1) && (loc_param2)) {
		int command_understood = HMI_command_parse(c, c->s1, set_commands, sizeof(set_commands)/sizeof(set_commands[0]), 0);
		if (command_understood)
			return command_understood;
		else {
			HMI_cprintf(c,"unknown config param\r\n");
			HMI_prompt(c);
		}
	} else {
		HMI_cprintf(c, "set command requires 2 param, first one can be one of\r\n");
		HMI_command_help(c, set_commands, sizeof(set_commands)/sizeof(set_commands[0]),1);
		return 2;
	}
	return 1;
}

unsigned long int HMI_str2IP(char* raw_string) {
	unsigned int IP_char_t[6];
	unsigned char IP_char[6]; 
	unsigned long int answer;
	int i;
	answer = sscanf(raw_string, "%i.%i.%i.%i", IP_char_t, IP_char_t+1, IP_char_t+2, IP_char_t+3);
	for (i=0;i<4; i++) {
		IP_char[i] = IP_char_t[i];
	}
	if (answer == 4) {
		answer = IP_char2int(IP_char);
		HMI_printf("OK\r\nready> ");
	} else {
		HMI_printf("bad IP format\r\nready> ");
		answer = 0;
	}
	return answer;
}

unsigned char HMI_yes_no_2int(char* raw_string) {
	unsigned char answer;
	if (strcmp (raw_string, "yes") == 0) {
		answer = 1;
	} 
	else if (strcmp (raw_string, "no") == 0) {
		answer = 0;
	}
	else {
		HMI_printf("value must be 'yes' or 'no'\r\nready> ");
		answer = -1;
	}
	return answer;
}

int HMI_cmd_who(struct context *c) {
	int i;
	unsigned int loc_age;
	unsigned int timer_snapshot;
	unsigned long int last_IP;
	unsigned char IP_c[6];
	char temp_string[50] = {0x1B,0x5B,0x41,0x1B,0x5B,0x41,0x1B,0x5B,0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x1B, 0x5B, 0x41,0x00};

	if (c->interrupt)
		return 1;
	if (!c->poll) {
		temp_string[0] = 0; 
	}
	HMI_cprintf (c,"%s%i Master: ID:127 Callsign:%s\r\n", temp_string, c->slow_counter, CONF_radio_master_callsign+2);
	IP_int2char (LAN_conf_applied.LAN_modem_IP, IP_c);
	HMI_cprintf (c,"ME: Callsign:%s ID:%i modem IP:%i.%i.%i.%i\r\n", CONF_radio_my_callsign+2, my_radio_client_ID, IP_c[0], IP_c[1], IP_c[2], IP_c[3]);
	HMI_cprintf (c,"Clients:\r\n");
	timer_snapshot = GLOBAL_timer.read_us();
	for (i=0; i<radio_addr_table_size; i++) {
		
	}
	for (i=0; i<radio_addr_table_size; i++) {
		loc_age = timer_snapshot - CONF_radio_addr_table_date[i];
		loc_age = loc_age / 1000000;
		if (is_TDMA_master) {loc_age = 0;} // master : already timeout in state machine
		//printf ("age:%i ", loc_age);
		if ( (CONF_radio_addr_table_status[i]) && (loc_age < connexion_timeout) ) {
			HMI_printf (" ID:%i Callsign:%s ", i, CONF_radio_addr_table_callsign[i]+2);
			IP_int2char (CONF_radio_addr_table_IP_begin[i], IP_c);
			HMI_printf ("IP start:%i.%i.%i.%i ", IP_c[0], IP_c[1], IP_c[2], IP_c[3]);
			last_IP = CONF_radio_addr_table_IP_begin[i] + CONF_radio_addr_table_IP_size[i] - 1;
			IP_int2char (last_IP, IP_c);
			HMI_printf ("IP end:%i.%i.%i.%i\r\n", IP_c[0], IP_c[1], IP_c[2], IP_c[3]);
		} else {
			HMI_printf ("                                                            \r\n");
		}
		
	}
	HMI_prompt_ctrlc(c);
	return 4;
}

int HMI_cmd_status(struct context *c) {
	static char text_radio_status[3][22] = {
		"waiting connection",
		"connected",
		"connection rejected"
	};
	static char text_reject_reason[2][15] = {
		"IP requested",
		"clients",
	};
	char temp_string[22];
	float loc_downlink_bw;
	float loc_uplink_bw;
	int TA_loc; 
	temp_string[0]=0x1B;
	temp_string[1]=0x5B;
	temp_string[2]=0x41;
	temp_string[3]=0x1B;
	temp_string[4]=0x5B;
	temp_string[5]=0x41;
	temp_string[6]=0x1B;
	temp_string[7]=0x5B;
	temp_string[8]=0x41;
	temp_string[9]=0x1B;
	temp_string[10]=0x5B;
	temp_string[11]=0x41;
	temp_string[12]=0x1B;
	temp_string[13]=0x5B;
	temp_string[14]=0x41;
	temp_string[15]=0x00;
	//temp_string[15]=0x1B;
	//temp_string[16]=0x5B;
	//temp_string[17]=0x41;
	//temp_string[18]=0x00;
	if (c->interrupt)
		return 1;
	if (!c->poll) { 
		temp_string[0] = 0; 
		G_downlink_bandwidth_temp = 0;
		G_uplink_bandwidth_temp = 0;
	}
	if (is_TDMA_master) {
		loc_downlink_bw = G_uplink_bandwidth_temp * 0.004;
		loc_uplink_bw = G_downlink_bandwidth_temp * 0.004;
		TA_loc = 0;
	} else {
		loc_downlink_bw = G_downlink_bandwidth_temp * 0.004;
		loc_uplink_bw =  G_uplink_bandwidth_temp * 0.004;
		TA_loc = TDMA_table_TA[my_radio_client_ID];
	}
	if (CONF_radio_state_ON_OFF == 0) {
		HMI_cprintf(c,"%s   %i status: radio OFF               \r\n", temp_string, c->slow_counter);
	}
	else if (my_client_radio_connexion_state == 0x03) {
		HMI_cprintf(c,"%s   %i status: rejected because too many %s \r\n", temp_string, c->slow_counter, text_reject_reason[G_connect_rejection_reason-2]);
	} else {
		HMI_cprintf(c,"%s   %i status: %s TA:%.1fkm Temp:%idegC   \r\n", temp_string, c->slow_counter, text_radio_status[my_client_radio_connexion_state-1], 0.15*(float)TA_loc, G_temperature_SI4463);
	}
	//HMI_printf("   TX buff filling %i\r\n", (TX_buff_ext_WR_pointer - TX_buff_ext_RD_pointer)*128);//!!!test
	if ( (is_TDMA_master) && (CONF_master_FDD == 2) ) {
		HMI_cprintf(c,"   RX tops from master FDD down %i\r\n", RX_top_FDD_up_counter);
	} else {
		HMI_printf("   RX_Eth_IPv4 %i ;TX_radio_IPv4 %i ; RX_radio_IPv4 %i   \r\n", RX_Eth_IPv4_counter, TX_radio_IPv4_counter, RX_radio_IPv4_counter);
		//HMI_printf("   RX_Eth_IPv4 %i ;TX_radio_IPv4 %i ; RX_radio_IPv4 %i   \r\n", RX_Eth_IPv4_counter, TX_radio_IPv4_counter, (TX_buff_ext_WR_pointer - TX_buff_ext_RD_pointer)*128);//!!!! debug FIFO filling
	}
	if ( (!is_TDMA_master) && (RSSI_stat_pkt_nb > 0) ) {
		//HMI_printf("RSSI: %i\r\nCTRL+c to exit...\r\n", (RSSI_total_stat / RSSI_stat_pkt_nb) );
		HMI_cprintf(c,"   DOWNLINK - bandwidth:%.1f RSSI:%.1f ERR:%.2f%%    \r\n", loc_downlink_bw, ((float)G_downlink_RSSI/256/2-136), ((float)G_downlink_BER)/500); // /500
		RSSI_total_stat = 0;
		RSSI_stat_pkt_nb = 0;
	} else {
		HMI_cprintf(c,"   DOWNLINK - bandwidth: %.1f RSSI:       ERR:      \r\n", loc_downlink_bw);
		
	}
	if ( (!is_TDMA_master) && (my_client_radio_connexion_state == 2) ) {
		HMI_cprintf(c,"   UPLINK -   bandwidth:%.1f RSSI:%.1f ERR:%.2f%%    \r\n", loc_uplink_bw, ((float)G_radio_addr_table_RSSI[my_radio_client_ID]/2-136), ((float)G_radio_addr_table_BER[my_radio_client_ID])/500);
	} else {
		HMI_cprintf(c,"   UPLINK -   bandwidth:%.1f RSSI:     ERR:      \r\n", loc_uplink_bw);
	}
	HMI_prompt_ctrlc(c);
	G_downlink_bandwidth_temp = 0;
	G_uplink_bandwidth_temp = 0;
	return 4;
}

void HMI_periodic_call (void) {
	if (ctx.ret == 4) {
		ctx.poll = 1;
		ctx.slow_counter++;
		ctx.ret=HMI_exec(&ctx);
		if (ctx.ret != 4)
			HMI_enable_echo();
	}
}

void HMI_periodic_fast_call (void) {
	if (ctx.ret == 5) {
		ctx.poll = 1;
		ctx.ret=HMI_exec(&ctx);
		if (ctx.ret != 5)
			HMI_enable_echo();
	}
}

void HMI_cwrite(struct context *ctx, const char *buffer, int size) {
	if (is_telnet_opened) {
		W5500_write_TX_buffer (W5500_p1, TELNET_SOCKET, (unsigned char*)buffer, size, 0);
	} else {
		fwrite(buffer,size,1,stdout);
		fflush(stdout);
	}
}

void HMI_printf_detail (const char *str) {
	HMI_cwrite(NULL, str, strlen(str));
}
