#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libnet.h>
#include <arpa/inet.h>

#define IFACE "eth0"

#define HW_TYPE	(0x0001) // Ethernet
#define IP_PROTO	(0x0800) // IP
#define HW_LEN	(6) // MAC adresa = 6B
#define IP_LEN	(4) // IP adresa = 4B
#define OPCODE_REQ	(1) // ARP ZIADOST
#define SRC_IP 	"192.168.1.1" // SEM vlozit lokalnu IP adresu
#define SRC_MAC "08:00:27:25:53:7f" // SEM vlozit MAC lokalneho rozhrania
#define DST_IP	"192.168.1.151"

#define ARP_TYPE	(0x0806)

#define OPCODE_RESP 	(2) // ARP ODPOVED

char err_buf[LIBNET_ERRBUF_SIZE];

int main (void){

	libnet_t * lc;
	libnet_ptag_t arp, eth;
	struct in_addr srcIP, dstIP;
	char srcMAC[6], dstMAC[6], bcastMAC[6];

	memset(bcastMAC, 0xFF, 6);

	memset(&srcIP, 0, sizeof(struct in_addr));
	if (inet_aton(SRC_IP, &srcIP) == 0){
		fprintf(stderr, "inet_aton(): Cannot convert text of SRC IP to IPv4 address.\n");
		exit(1);
	}

	memset(&dstIP, 0, sizeof(struct in_addr));
	if (inet_aton(DST_IP, &dstIP) == 0){
		fprintf(stderr, "inet_aton(): Cannot convert text of DST IP to IPv4 address.\n");
		exit(1);
	}

	if ((lc = libnet_init(LIBNET_LINK, IFACE, err_buf)) == NULL){
		fprintf(stderr, "ERROR: libnet_init()\n");
		exit(1);
	}

	arp = libnet_autobuild_arp(OPCODE_REQ, (const uint8_t *) srcMAC, (const uint8_t *) &(srcIP.s_addr), (const uint8_t *) dstMAC, (uint8_t *)&(dstIP.s_addr), lc);
	if (arp == -1){
		fprintf(stderr, "ERROR: libnet_build_arp(): %s\n", libnet_geterror(lc));
		exit(1);
	}

	eth = libnet_autobuild_ethernet((const uint8_t *) bcastMAC, ARP_TYPE, lc);
	if (eth == -1){
		fprintf(stderr, "ERROR: libnet_build_ethernet(): %s\n", libnet_geterror(lc));
		exit(1);
	}

	for (int i = 0; i < 5; i++){
		if (libnet_write(lc) == -1){
			fprintf(stderr, "ERROR: libnet_write(): %s\n", libnet_geterror(lc));
			exit(1);
		}
	}

	libnet_destroy(lc);
	return 0;
}
