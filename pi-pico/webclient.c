#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "tusb_lwip_glue.h"

#include "lwip/tcp.h"

#define LED_PIN     25

uint32_t last;

err_t error;

struct tcp_pcb *testpcb;



err_t tcpRecvCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    gpio_put(LED_PIN, 0);

    printf("Data received: %dus\n",time_us_32()-last);
    last = time_us_32();

    if (p == NULL) {
        printf("The remote host closed the connection.\n");
        printf("Now I'm closing the connection.\n");
        //tcp_close_con();
        tcp_close(tpcb);
        return ERR_ABRT;
    } else {
        printf("Number of pbufs %d\n", pbuf_clen(p));
        printf("Contents of pbuf %.*s\n", p->len, (char *)p->payload);
	if (p->len && p->payload) {
	   char *c=(char *)p->payload;
	   if (c[0] == 'x') {
              tcp_close(tpcb);
	      reset_usb_boot(0,0);
           }
        }
    }

    return 0;
}

uint32_t tcp_send_packet(struct tcp_pcb *pcb)
{
    char *string = "Hello\r\n";
    uint32_t len = strlen(string);

    gpio_put(LED_PIN, 1);

    printf("tcp_send_packet(%s): %dus\n", string, time_us_32()-last);
    last = time_us_32();

    /* push to buffer */
    err_t    error = tcp_write(pcb, string, strlen(string), TCP_WRITE_FLAG_COPY);

    if (error) {
        printf("ERROR: Code: %d (tcp_send_packet :: tcp_write)\n", error);
        return 1;
    }

    /* now send */
    error = tcp_output(pcb);
    if (error) {
        printf("ERROR: Code: %d (tcp_send_packet :: tcp_output)\n", error);
        return 1;
    }
    return 0;
}

err_t tcpPollCallback(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err = ERR_OK;
  static int sent;
  if (!sent) {
      tcp_send_packet(tpcb);
      sent=1;
  }
  return ret_err;
}


err_t tcpSendCallback(void* arg, struct tcp_pcb *pcb, u16_t len)
{
    gpio_put(LED_PIN, 0);
    printf("tcpSendCallback(): %dus\n", time_us_32()-last);
    last = time_us_32();
    return 0;
}

void tcpErrorHandler(void* arg, err_t err)
{
    printf("tcpErrorHandler()=%d: %dus\n", err, time_us_32()-last);

    for(;;)  { gpio_xor_mask(1 << LED_PIN);  sleep_ms(250); }
}


err_t tcpAcceptCallback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  uint32_t data = 0xdeadbeef;
  void *es = &data;

  LWIP_UNUSED_ARG(arg);
  if ((err != ERR_OK) || (newpcb == NULL)) {
    return ERR_VAL;
  }

  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  if (es != NULL) {
    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_err(newpcb, tcpErrorHandler);
    tcp_recv(newpcb, tcpRecvCallback);
    tcp_sent(newpcb, tcpSendCallback);
    tcp_poll(newpcb, tcpPollCallback, 0);
    tcp_send_packet(newpcb); 
    ret_err = ERR_OK;
  } else {
    ret_err = ERR_MEM;
  }
  return ret_err;
}


void tcp_setup(void)
{
    err_t err;

    printf("tcp_setup(): %dus\n",time_us_32()-last);
    last = time_us_32();

    gpio_put(LED_PIN, 1);

    /* create the control block */
    testpcb = tcp_new();    //testpcb is a global struct tcp_pcb
                            // as defined by lwIP



    err = tcp_bind(testpcb, IP_ANY_TYPE, 23);
    if (err == ERR_OK) {
        testpcb = tcp_listen(testpcb);
        tcp_accept(testpcb, tcpAcceptCallback);
    } else {
        /* abort? output diagnostic? */
    }


}

int main()
{
    stdio_uart_init();

    last = time_us_32();

    // Initialize tinyusb, lwip, dhcpd and httpd
    init_lwip();
    printf("init_lwip(): %dus\n",time_us_32()-last);
    last = time_us_32();
    wait_for_netif_is_up();
    printf("wait_for_netif_is_up(): %dus\n",time_us_32()-last);
    last = time_us_32();
    dhcpd_init();
    printf("dhcpd_init(): %dus\n",time_us_32()-last);
    last = time_us_32();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(LED_PIN, 1);
    tcp_setup();

    while (true)
    {
        tud_task();
        service_traffic();
    }

    return 0;
}
