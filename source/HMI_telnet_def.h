#ifndef HMI_TELNET_DEF_H
#define HMI_TELNET_DEF_H
extern char HMI_out_str[120];

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

enum retcode {
  RET_ERROR=-1,
  RET_UNKNOWN=0,
  RET_SILENT=1,
  RET_PROMPT=2,
  RET_OK_PROMPT=3,
  RET_POLL_SLOW=4,
  RET_POLL_FAST=5,
};

struct command {
  const char *cmd;
  enum retcode (*func)(struct context *ctx);
};

struct error {
  int code;
  const char *text;
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

enum retcode HMI_command_parse(struct context *ctx, const char *s, const struct command *cmd, int len, int help);



int HMI_check_radio_OFF(void);

void HMI_TX_test(char* duration_txt);

void HMI_reboot(void);

void HMI_force_exit(void);

void HMI_exit(void);

unsigned long int HMI_str2IP(char* raw_string);

unsigned char HMI_yes_no_2int(char* raw_string);

void HMI_periodic_call(void);

void HMI_periodic_fast_call(void);

void HMI_printf_detail (const char *str);
void HMI_cwrite(struct context *c, const char *buffer, int size);
#endif
