#include "W5500_lwip.h"
#include "../source/W5500.h"

struct W5500_channel W5500_channel[NR_SOCKETS];

struct pbuf *
W5500_dequeue(struct W5500_channel *c, enum W5500_dir dir)
{
	struct pbuf *ret=c->pbuf[dir];
	if (ret) {
        	c->pbuf[dir]=pbuf_dechain(c->pbuf[dir]);
	}
	return ret;
}

void
W5500_enqueue(struct W5500_channel *c, enum W5500_dir dir, unsigned char *data, int size)
{
	struct pbuf *p=pbuf_alloc(PBUF_RAW, size, PBUF_RAM);
	memcpy(p->payload, data, size);
	if (c->pbuf[dir]) {
		pbuf_cat(c->pbuf[dir], p);
	} else {
		c->pbuf[dir]=p;
	}
}

int
W5500_next_size(struct W5500_channel *c, enum W5500_dir dir)
{
	if (!c->pbuf[dir])
		return 0;
	return c->pbuf[dir]->len;
}

err_t
W5500_transmit(struct W5500_channel *c)
{
	err_t err = ERR_OK;
	struct pbuf *p=W5500_dequeue(c, TX);
	if (p) {
		err = tcp_write(c->conn, p->payload, p->len, TCP_WRITE_FLAG_COPY);
		if (!err) {
			err=tcp_output(c->conn);
		}
                pbuf_free(p);
	}
	return err;
}

uint8_t
W5500_read_byte(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr)
{
	return 0;
}

void
W5500_write_byte(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr, unsigned char data)
{
}

void
W5500_read_long(W5500_chip* SPI_p_loc, unsigned int W5500_addr, unsigned char bloc_addr, unsigned char* RX_data, int RX_size)
{
}

void
W5500_write_TX_buffer(W5500_chip* SPI_p_loc, uint8_t sock_nb, unsigned char* data, int size, int send_mac) 
{
}

void
W5500_read_RX_buffer(W5500_chip* SPI_p_loc, uint8_t sock_nb, uint8_t* data, int size)
{
}

uint16_t
W5500_read_received_size(W5500_chip* SPI_p_loc, uint8_t sock_nb)
{
	return 0;
}
