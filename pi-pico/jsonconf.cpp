#include <stdarg.h>
#include "mbed.h"
#include "pico_hal.h"
#include "../source/global_variables.h"
#include "../source/HMI_telnet.h"
#include "../source/config_flash.h"
#include "../source/TDMA.h"

enum type {
	TYPE_BOOL,
	TYPE_UINT8,
	TYPE_UINT16,
	TYPE_UINT32,
	TYPE_IP,
	TYPE_FREQUENCY,
	TYPE_SHIFT,
	TYPE_STRING13,
};

#define CONF_BOOL(name,var,def) {#name,TYPE_BOOL,&var,def}
#define CONF_UINT8(name,var,def) {#name,TYPE_UINT8,&var,def}
#define CONF_UINT16(name,var,def) {#name,TYPE_UINT16,&var,def}
#define CONF_UINT32(name,var,def) {#name,TYPE_UINT32,&var,def}
#define CONF_IP(name,var,def) {#name,TYPE_IP,&var,def}
#define CONF_FREQUENCY(name,var,def) {#name,TYPE_FREQUENCY,&var,def}
#define CONF_SHIFT(name,var,def) {#name,TYPE_SHIFT,&var,def}
#define CONF_STRING13(name,var,def) {#name,TYPE_STRING13,var,def}

static uint8_t internal_modulation;
static float internal_frequency;
static float internal_shift;
static uint16_t internal_mac_ls_bytes;

struct config {
	const char *name;
	enum type type;
	void *dest;
	const char *deflt;
} config[]={
/* OK */
	CONF_BOOL(is_master,is_TDMA_master,"false"),
	CONF_STRING13(callsign,CONF_radio_my_callsign+2,"N0CALL-1       "),
	CONF_BOOL(telnet_last_active,is_telnet_active,"true"),
	CONF_UINT8(modulation,internal_modulation,"24"),
	CONF_FREQUENCY(frequency,internal_frequency,"437.000"),
	CONF_UINT8(radio_network_id,CONF_radio_network_ID,"0"),
	CONF_BOOL(client_static_ip,CONF_radio_static_IP_requested,"false"),
	CONF_UINT32(client_req_size,CONF_radio_IP_size_requested,"1"),
	CONF_BOOL(dhcp_active,LAN_conf_saved.DHCP_server_active,"true"),
	CONF_BOOL(master_fdd,CONF_master_FDD,"false"),
	CONF_IP(modem_ip,LAN_conf_saved.LAN_modem_IP,"192.168.0.253"),
	CONF_IP(netmask,LAN_conf_saved.LAN_subnet_mask,"255.255.255.0"),
	CONF_UINT32(ip_size,CONF_radio_IP_size,"32"),
	CONF_BOOL(dns_active,LAN_conf_saved.LAN_DNS_activ,"true"),
	CONF_IP(dns_value,LAN_conf_saved.LAN_DNS_value,"9.9.9.9"),
	CONF_BOOL(def_route_active,LAN_conf_saved.LAN_def_route_activ,"true"),
	CONF_IP(default_route,LAN_conf_saved.LAN_def_route,"192.168.0.1"),
	CONF_IP(ip_start,CONF_radio_IP_start,"192.168.0.65"),
	CONF_BOOL(telnet_routed,is_telnet_routed,"true"),
	CONF_UINT16(mac_ls_bytes,internal_mac_ls_bytes,"0"),
	CONF_BOOL(radio_on_at_start,CONF_radio_default_state_ON_OFF,"false"),
	CONF_UINT8(rf_power,CONF_radio_PA_PWR,"127"),
	// CONF_UINT16(checksum,CONF_checksum,0),
	CONF_SHIFT(shift,internal_shift,"0.000"),
	CONF_IP(master_down_ip,CONF_master_down_IP,"192.168.0.252"),
};

struct json_doc {
	const char *buffer;
};

static int
json_read(struct json_doc *doc, const char *filename, char *buffer, int size)
{
	int rsize,file=pico_open(filename, LFS_O_RDONLY);
        if (file < 0)
                return file;
	rsize=pico_read(file, buffer, size);
	pico_close(file);
	if (rsize >= size-1)
		return LFS_ERR_FBIG;
	buffer[rsize]='\0';
	doc->buffer=buffer;
	return 0;
}

static int
json_get(struct json_doc *doc, const char *key, char *value, int value_size)
{
	const char *pos=doc->buffer;
	const char *val=NULL;
	const char *end=NULL;
	int j;

	while (pos=strstr(pos+1,key)) {
		int len=strlen(key);
		if (pos[-1] == '"' && pos[len] == '"') {
			pos+=len+1;
			while(*pos == ' ')
				pos++;
			if (*pos == ':') {
				pos++;
				while (*pos == ' ')
					pos++;
				val=pos;
			}
		}
	}
	if (!val)
		return LFS_ERR_NOENT;
	if (*val == '"') {
		val++;
		end=strchr(val,'"');
	} else {
		for (j = 0 ; j < 3 ; j++) {
			char s=",\r\n"[j];
			char *tmp=strchr(val,s);
			if (!end || tmp < end)
				end=tmp;
		}
	}
	if (!end)
		return LFS_ERR_CORRUPT;
	if (end-val >= value_size)
		return LFS_ERR_FBIG;
	strncpy(value, val, end-val);
	value[end-val]='\0';
	return 0;
}


int
config_read(char *buffer, int size, AnalogIn* analog_pin)
{
	char value[32];
	const char *s;
	int err,i,callsign_present=0,do_save=0;
	unsigned char u8,modul_temp;
	unsigned short u16;
	unsigned int u32,ip[4];
	float f;
	struct json_doc doc;
	err=json_read(&doc,"config.json",buffer,size);
	for (i = 0 ; i < sizeof(config)/sizeof(config[0]) ; i++) {
		const char *name=config[i].name;
		const char *val=NULL;
		const void *ptr;
		int size=0;
		if (!json_get(&doc, name, value, sizeof(value))) 
			val=value;
		ptr=config[i].deflt;
		s=val?val:config[i].deflt;
		switch(config[i].type) {
		case TYPE_BOOL:
			size=1;
			u8=!strcmp(s,"true");
			ptr=&u8;
			break;
		case TYPE_UINT8:
			size=1;
			u8=atoi(s);
			ptr=&u8;
			break;
		case TYPE_UINT16:
			size=2;
			u16=atoi(s);
			ptr=&u16;
			break;
		case TYPE_UINT32:
			size=4;
			u32=atoi(s);
			ptr=&u32;
			break;
		case TYPE_IP:
			if (val)
				ptr=val;
			break;
		case TYPE_FREQUENCY:
		case TYPE_SHIFT:
			size=4;
			f=atof(s);
			ptr=&f;
			break;
		case TYPE_STRING13:
			size=13;
			if (val) {
				ptr=val;
				callsign_present=1;
			}
			break;
		}
		if (config[i].type == TYPE_IP) {
			int res = sscanf((const char *)ptr, "%i.%i.%i.%i", ip, ip+1, ip+2, ip+3);
			if (res == 4) {
				unsigned int ipval=ip[3]|(ip[2]<<8)|(ip[1]<<16)|(ip[0]<<24);
				memcpy(config[i].dest,&ipval,4);
			}
		} else {
			memcpy(config[i].dest,ptr,size);
		}
	}

	modul_temp = (internal_modulation & 0x3F);
	if ( ((modul_temp>=11)&&(modul_temp<=14)) || ((modul_temp>=20)&&(modul_temp<=24)) ) {
		CONF_radio_modulation = modul_temp;
	} else {
		CONF_radio_modulation = 24;
	}
	CONF_frequency_band = (internal_modulation & 0xC0) >> 6;

	if (internal_mac_ls_bytes == 0x0 || internal_mac_ls_bytes == 0xffff) {
		internal_mac_ls_bytes = (NFPR_random_generator(analog_pin) << 8) + NFPR_random_generator(analog_pin);
		debug("mac bytes 0x%x\r\n",internal_mac_ls_bytes);
		do_save=1;
	}

        CONF_modem_MAC[0] = 0x4E;//N
        CONF_modem_MAC[1] = 0x46;//F
        CONF_modem_MAC[2] = 0x50;//P
        CONF_modem_MAC[3] = 0x52;//R
        CONF_modem_MAC[4] = internal_mac_ls_bytes >> 8;
        CONF_modem_MAC[5] = internal_mac_ls_bytes & 0xff;

	if (callsign_present) {
		CONF_radio_my_callsign[0] = CONF_modem_MAC[4];
		CONF_radio_my_callsign[1] = CONF_modem_MAC[5];
	}
	CONF_radio_my_callsign[15] = '\0';

        CONF_frequency_HD = (internal_frequency-FREQ_RANGE_MIN)*1000;
	debug("freq %f %d %d %d\r\n",internal_frequency,CONF_frequency_HD,FREQ_RANGE_MIN,FREQ_MAX_RAW);
        if ( (CONF_frequency_HD == 0x0000) || (CONF_frequency_HD > FREQ_MAX_RAW) ) {
                CONF_frequency_HD = CONF_DEF_FREQ; // force to default frequency
        }
	CONF_freq_shift = internal_shift*1000;
	LAN_conf_applied=LAN_conf_saved;
        if ( (is_TDMA_master) && (CONF_master_FDD == 1) ) { // FDD Master down
                G_FDD_trig_pin->output();
        }
        if ( (is_TDMA_master) && (CONF_master_FDD == 2) ) {// FDD master up
                G_FDD_trig_IRQ->rise(&TDMA_FDD_up_top_measure);
        }
	if (is_TDMA_master) {
		my_client_radio_connexion_state = 2;
	} else {
		my_client_radio_connexion_state = 1;
		my_radio_client_ID = 0x7E;
        }
	return 0;
}

void
config_write(int file, const char *str, ...)
{
	va_list ap;
        char buffer[128];
        va_start(ap, str);
        vsnprintf(buffer, sizeof(buffer), str, ap);
	if (file)
		pico_write(file, buffer, strlen(buffer));
	else
		HMI_cwrite(NULL, buffer, strlen(buffer));
        va_end(ap);
}

unsigned char NFPR_random_generator(AnalogIn* analog_pin) {
        unsigned short interm_random;
        unsigned char random_8;
        int i;
        random_8 = 0;
        for (i=0; i<8; i++) {
                interm_random = analog_pin->read_u16();
                interm_random = (interm_random & 0x10)>>4;
                interm_random = (interm_random << i);
                random_8 = random_8 + interm_random;
                wait_ms(4);
        }
        return random_8;
}

unsigned int
NFPR_config_save(void)
{
	int i,diff;
	char val[32];
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	float fl;
	char *s;
	unsigned char *ip;
	int first=1;

        if ( (CONF_radio_my_callsign[0] == 0) || (CONF_radio_my_callsign[2] == 0) ) {
                HMI_printf("ERROR : not yet configured\r\n");
		return -1;
	}

#if 0
	int file=pico_open("config.json",LFS_O_CREAT|LFS_O_RDWR|LFS_O_TRUNC);
#else
	int file=0;
#endif
	
	internal_modulation=( (CONF_frequency_band << 6) & 0xC0) + (CONF_radio_modulation & 0x3F);
	internal_shift=CONF_freq_shift/1000;
	internal_frequency=((float)CONF_frequency_HD/1000)+FREQ_RANGE_MIN;
	internal_mac_ls_bytes=CONF_modem_MAC[4] * 256 + CONF_modem_MAC[5];
	
	config_write(file,"{");
	for (i = 0 ; i < sizeof(config)/sizeof(config[0]) ; i++) {
		const char *quote="";
		val[0]='\0';
		switch(config[i].type) {
		case TYPE_BOOL:
		case TYPE_UINT8:
			u8=*((uint8_t *)config[i].dest);
			if (config[i].type == TYPE_UINT8)
				snprintf(val,sizeof(val),"%d",u8);
			else
				strcpy(val,u8?"true":"false");
			break;
		case TYPE_UINT16:
			u16=*((uint16_t *)config[i].dest);
			snprintf(val,sizeof(val),"%d",u16);
			break;
		case TYPE_UINT32:
			u32=*((uint32_t *)config[i].dest);
			snprintf(val,sizeof(val),"%d",u32);
			break;
		case TYPE_IP:
			quote="\"";
			ip=(unsigned char *)config[i].dest;
			snprintf(val,sizeof(val),"%d.%d.%d.%d",ip[3],ip[2],ip[1],ip[0]);
			break;
		case TYPE_FREQUENCY:
		case TYPE_SHIFT:
			fl=*(float *)config[i].dest;
			snprintf(val,sizeof(val),"%.3f",fl);
			break;
		case TYPE_STRING13:
			quote="\"";
			s=(char *)config[i].dest;
			snprintf(val,sizeof(val),"%s",s);
			break;
		}
		if (strcmp(val,config[i].deflt)) {
			config_write(file,"%s\r\n\"%s\": %s%s%s",first?"":",",config[i].name,quote,val,quote);
			first=0;
		}
	}
	config_write(file,"\r\n}\r\n");
	if (file) {
		pico_fflush(file);
		pico_close(file);
	}
	return 0;
}

void NFPR_config_read(AnalogIn* analog_pin) {
	config_read((char *)int_sram,INT_SRAM_SIZE,analog_pin);
        if (is_TDMA_master) {
                my_client_radio_connexion_state = 2;
        } else {
                my_client_radio_connexion_state = 1;
                my_radio_client_ID = 0x7E;
        }
}
