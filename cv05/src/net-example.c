#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libnet.h>

#define IFACE "eth0"
#define SRC_MAC "08:00:27:9d:9b:81"
#define SRC_IP "10.0.2.15"
#define TARGET_IP "10.0.2.1"

char error[LIBNET_ERRBUF_SIZE];

int main(void){

	libnet_t * lc;
	uint8_t srcMAC[6], dstMAC[6], bcastMAC[6];
	struct in_addr srcIP, dstIP;

	libnet_ptag_t arp, eth;

	memset(&srcIP, 0, sizeof(struct in_addr));
	if (inet_aton(SRC_IP, &srcIP) == 0){
		fprintf(stderr, "inet_aton() : Cannot convert text of SRC IP to IPv4 address.\n");
		return EXIT_FAILURE;
	}

	memset(&srcMAC, 0, sizeof(srcMAC));
	sscanf(SRC_MAC, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &srcMAC[0], &srcMAC[1], &srcMAC[2], &srcMAC[3], &srcMAC[4], &srcMAC[5]);

	memset(&dstMAC, 0, sizeof(dstMAC));
	memset(&bcastMAC, 0xFF, sizeof(bcastMAC));

	memset(&dstIP, 0, sizeof(struct in_addr));
	if (inet_aton(TARGET_IP, &dstIP) == 0){
		fprintf(stderr, "inet_aton() : Cannot convert text of SRC IP to IPv4 address.\n");
		return EXIT_FAILURE;
	}

	if ((lc = libnet_init(LIBNET_LINK, IFACE, error)) == NULL){
		fprintf(stderr, "libnet_init() : %s\n", error);
		return EXIT_FAILURE;
	}

	arp = libnet_autobuild_arp(ARPOP_REQUEST, (const uint8_t *) srcMAC, (const uint8_t *) &(srcIP.s_addr), (const uint8_t *) dstMAC, (uint8_t *) &(dstIP.s_addr), lc);
	if (arp == -1){
		fprintf(stderr, "libnet_autobuild_arp() : Error occured.\n");
		return EXIT_FAILURE;
	}

	eth = libnet_autobuild_ethernet((const uint8_t *) bcastMAC, ETHERTYPE_ARP, lc);
	if (eth == -1){
		fprintf(stderr, "libnet_autobuild_ethernet() : Error occured.\n");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < 10; i++){
		if (libnet_write(lc) == -1){
			fprintf(stderr, "libnet_write() : ARP request was NOT sent.\n");
			return EXIT_FAILURE;
		}
	}

	libnet_destroy(lc);

	return EXIT_SUCCESS;
}
