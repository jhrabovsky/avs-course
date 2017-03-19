#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <pcap/pcap.h>

#define IFACE	"eth0"

char err_buf[PCAP_ERRBUF_SIZE];

void spracuj(u_char * user, const struct pcap_pkthdr * pkthdr, const u_char * pkt){
	printf("Frame received at [%d] with length [%d]\n", pkthdr->ts.tv_sec, pkthdr->len);
}

int main(void){
	pcap_t * pcap;

	if ((pcap = pcap_create((const char *)IFACE, err_buf)) == NULL){
		fprintf(stderr, "pcap_create() : %s\n", err_buf);
		exit(1);
	}

	pcap_set_promisc(pcap,1);

	pcap_activate(pcap);

	if (pcap_loop(pcap, 0, spracuj, NULL) == -1){
		pcap_perror(pcap, "pcap_loop()");
		exit(1);
	}

	pcap_close(pcap);
	return 0;
}
