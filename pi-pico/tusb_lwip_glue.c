/* 
 * The MIT License (MIT)
 *
 * Based on tinyUSB example that is: Copyright (c) 2020 Peter Lawrence
 * Modified for Pico by Floris Bos
 *
 * influenced by lrndis https://github.com/fetisov/lrndis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb_lwip_glue.h"
#include "pico/unique_id.h"

/* lwip context */
struct netif netif_usb;

/* shared between tud_network_recv_cb() and service_traffic() */
static struct pbuf *received_frame;

/* this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c */
/* ideally speaking, this should be generated from the hardware's unique ID (if available) */
/* it is suggested that the first byte is 0x02 to indicate a link-local address */
const uint8_t tud_network_mac_address[6] = {0x02,0x02,0x84,0x6A,0x96,0x00};

/* network parameters of this MCU */
static const ip_addr_t ipaddr  = IPADDR4_INIT_BYTES(192, 168, 7, 1);
static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t gateway = IPADDR4_INIT_BYTES(192, 168, 7, 2);

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
    (void)netif;
    
    for (;;)
    {
      /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing we can do */
      if (!tud_ready())
        return ERR_USE;
    
      /* if the network driver can accept another packet, we make it happen */
      if (tud_network_can_xmit(p->len))
      {
        tud_network_xmit(p, 0 /* unused for this example */);
        return ERR_OK;
      }
    
      /* transfer execution to TinyUSB in the hopes that it will finish transmitting the prior packet */
      tud_task();
    }
}

static err_t output_fn(struct netif *netif, struct pbuf *p, const ip_addr_t *addr)
{
    return etharp_output(netif, p, addr);
}

static err_t netif_init_cb(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'u';
    netif->name[1] = 's';
    netif->linkoutput = linkoutput_fn;
    netif->output = output_fn;
    return ERR_OK;
}

void tud_setup(void)
{
    struct netif *netif = &netif_usb;
    
    /* Fixup MAC address based on flash serial */
    //pico_unique_board_id_t id;
    //pico_get_unique_board_id(&id);
    //memcpy( (tud_network_mac_address)+1, id.id, 5);
    // Fixing up does not work because tud_network_mac_address is const
    
    /* Initialize tinyUSB */
    tusb_init();
   
    /* the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbit */
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
    netif->hwaddr[5] ^= 0x01;
   
#if 0 
    netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb, ip_input);
    netif_set_default(netif);
#else
    netif = netif_add_noaddr(netif, NULL, netif_init_cb, ip_input);
#endif
}

void tud_network_init_cb(void)
{
    /* if the network is re-initializing and we have a leftover packet, we must do a cleanup */
    if (received_frame)
    {
      pbuf_free(received_frame);
      received_frame = NULL;
    }
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
    /* this shouldn't happen, but if we get another packet before 
    parsing the previous, we must signal our inability to accept it */
    if (received_frame) return false;
    
    if (size)
    {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if (p)
        {
            /* pbuf_alloc() has already initialized struct; all we need to do is copy the data */
            memcpy(p->payload, src, size);
        
            /* store away the pointer for service_traffic() to later handle */
            received_frame = p;
        }
    }

    return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
    struct pbuf *p = (struct pbuf *)ref;
    struct pbuf *q;
    uint16_t len = 0;

    (void)arg; /* unused for this example */

    /* traverse the "pbuf chain"; see ./lwip/src/core/pbuf.c for more info */
    for(q = p; q != NULL; q = q->next)
    {
        memcpy(dst, (char *)q->payload, q->len);
        dst += q->len;
        len += q->len;
        if (q->len == q->tot_len) break;
    }

    return len;
}

void service_traffic(void)
{
    /* handle any packet received by tud_network_recv_cb() */
    if (received_frame)
    {
      netif_usb.input(received_frame, &netif_usb);
      received_frame = NULL;
      tud_network_recv_renew();
    }
    
    sys_check_timeouts();
}
