#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <pcap/pcap.h>

#define ANY_IFACE "any"
#define IFACE "eth0"
#define CAPTURE_LEN (100)

char error[PCAP_ERRBUF_SIZE];

int main(void){

	pcap_t * pcap;

    // Zobrazim INFO o dostupnych rozhraniach pre PCAP
    /*
	pcap_if_t * pcap_ifaces;

	if (pcap_findalldevs(&pcap_ifaces, error) == -1){
		fprintf(stderr, "pcap_findalldevs() : %s\n", error);
		return EXIT_FAILURE;
	}

	pcap_if_t * iter = pcap_ifaces;
	while(iter != NULL){
		printf("IFACE [%s][%s %s] : [%s]\n", iter->name, (iter->flags & PCAP_IF_UP)?"UP":"DOWN", (iter->flags & PCAP_IF_RUNNING)?"running":"", (iter->description != NULL)?iter->description:"Unknown");
		iter = iter->next;
	}
	pcap_freealldevs(pcap_ifaces);
	*/


	pcap = pcap_create(IFACE, error);
	if (pcap == NULL){
		fprintf(stderr, "pcap_create() : %s\n", error);
		return EXIT_FAILURE;
	}

	pcap_set_snaplen(pcap, CAPTURE_LEN);
	pcap_set_promisc(pcap, 1);

	if (pcap_activate(pcap) < 0){
		fprintf(stderr, "pcap_activate() : Error occured.\n");
		return EXIT_FAILURE;
	}

	const u_char * pkt = NULL;
	struct pcap_pkthdr hdr;
	while((pkt = pcap_next(pcap, &hdr)) != NULL){
		printf("%ld.%06ld - packet captured with length %d [available length %d]\n", hdr.ts.tv_sec, hdr.ts.tv_usec, hdr.len, hdr.caplen);
	}

	pcap_close(pcap);

	return EXIT_SUCCESS;
}
