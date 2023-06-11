#include <glib.h>
#include <unistd.h>

typedef struct pcap_hdr_s {
	guint32 magic_number;	/* magic number */
	guint16 version_major;	/* major version number */
	guint16 version_minor;	/* minor version number */
	gint32 thiszone;	/* GMT to local correction */
	guint32 sigfigs;	/* accuracy of timestamps */
	guint32 snaplen;	/* max length of captured packets, in octets */
	guint32 network;	/* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
	guint32 ts_sec;		/* timestamp seconds */
	guint32 ts_usec;	/* timestamp microseconds */
	guint32 incl_len;	/* number of octets of packet saved in file */
	guint32 orig_len;	/* actual length of packet */
} pcaprec_hdr_t;

static int written;

static void output(void *buffer, int len)
{
	write(1, buffer, len);
	written+=len;
}

int main(int argc, char **argv)
{
	pcap_hdr_t hdr;
	pcaprec_hdr_t rec_hdr;
	unsigned char data[64];
	hdr.magic_number=0xa1b2c3d4;
	hdr.version_major=2;
	hdr.version_minor=4;
	hdr.thiszone=0;
	hdr.sigfigs=0;
	hdr.snaplen=65535;
	hdr.network=1;
	write(1, &hdr, sizeof(hdr));
	rec_hdr.ts_sec=0;
	rec_hdr.ts_usec=0;
	rec_hdr.incl_len=64;
	rec_hdr.orig_len=64;
	write(1, &rec_hdr, sizeof(rec_hdr));
	int ts=1234;
	int us=5678;

	output((unsigned char []){0x00,0x00,0x00,0x00,0x00,0x00}, 6);
	output((unsigned char []){0x00,0x00,0x00,0x00,0x00,0x00}, 6);
	output((unsigned char []){0xff,0xff}, 2);
	output((unsigned char []){0}, 1);  /* type */
	output((unsigned char []){1}, 1);  /* sequence */
	output(&us, 4);  /* us */
	output(&ts, 3);  /* rx_timer */
	output((unsigned char []){4}, 1);  /* rssi */
	output((unsigned char []){5}, 1);  /* length */
	output((unsigned char []){6}, 1);  /* TDMA byte */
	output((unsigned char []){7}, 1);  /* client it byte */
	output((unsigned char []){8}, 1);  /* protocol byte */
	output((unsigned char []){0x45}, 1);  /* ip version ihl */
	output((unsigned char []){0}, 1);  /* DSCP/ECN */
	output((unsigned char []){0,36}, 2);  /* lengh */
	output((unsigned char []){0,0}, 2);  /* identification */
	output((unsigned char []){0,0}, 2);  /* fragment offset */
	output((unsigned char []){64}, 1);  /* ttl */
	output((unsigned char []){17}, 1);  /* protocol */
	output((unsigned char []){0,0}, 2);  /* checksum */
	output((unsigned char []){0,0,0,0}, 4);  /* src */
	output((unsigned char []){0,0,0,0}, 4);  /* dst */

	output((unsigned char []){0,0}, 2);  /* src port */
	output((unsigned char []){0,0}, 2);  /* dst port */
	output((unsigned char []){0,8}, 2);  /* len */
	output((unsigned char []){0,0}, 2);  /* chksum */

	while (written < 64) {
		output((unsigned char []){0}, 1);  /* padding */
	}
#if 0
	data[22]=0x9; /* protocol byte */
	data[23]=0x45; /* ip version iphl */
	data[32]=17; /* protocol */
	data[48]=8; /* length */
	write(1, data, sizeof(data));
#endif
}
