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

#ifndef TELNET_F4
#define TELNET_F4
#ifdef __cplusplus
#include "mbed.h"
#include "W5500.h"
#include "global_variables.h"

int telnet_loop(W5500_chip* W5500);

#else
extern char HMI_out_str[120];
#endif

struct context {
  char *cmd;
  char *s1;
  char *s2;
  int poll;
  int interrupt;
  int slow_counter;
  int ret;
  void *async_data;
};

struct command {
  const char *cmd;
  int (*func)(struct context *ctx);
};

#define HMI_printf(param, ...) do {snprintf (HMI_out_str, sizeof(HMI_out_str), param, ##__VA_ARGS__);\
	HMI_printf_detail(HMI_out_str);} while(0)

#define HMI_cprintf(ctx, param, ...) do {snprintf (HMI_out_str, sizeof(HMI_out_str), param, ##__VA_ARGS__);\
	HMI_cwrite(ctx, HMI_out_str, strlen(HMI_out_str));} while(0)

void HMI_prompt(struct context *c);

void HMI_close_telnet(void);

int serial_term_loop (void);

void HMI_line_parse (struct context *c, char* RX_text, int RX_text_count);

void HMI_cancel_current(struct context *c);

int HMI_command_parse(struct context *ctx, const char *s, const struct command *cmd, int len, int help);



int HMI_check_radio_OFF(void);

void HMI_TX_test(char* duration_txt);

void HMI_reboot(void);

void HMI_force_exit(void);

void HMI_exit(void);

unsigned long int HMI_str2IP(char* raw_string);

unsigned char HMI_yes_no_2int(char* raw_string);

void HMI_periodic_call(void);

void HMI_periodic_fast_call(void);

#ifdef __cplusplus
extern "C" {
#endif
void HMI_printf_detail (const char *str);
void HMI_cwrite(struct context *c, const char *buffer, int size);
#ifdef __cplusplus
}
#endif

#endif
