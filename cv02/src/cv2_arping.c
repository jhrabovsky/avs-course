#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include <arpa/inet.h>
#include <net/if.h>

#include <netinet/in.h>

#define ERROR	(0)
#define SUCCESS	(1)

#define IFACE	"eth0"
#define SRC_IP 	"192.168.56.10" // SEM vlozit lokalnu IP adresu
#define SRC_MAC "08:00:27:bc:ef:24" // SEM vlozit MAC lokalneho rozhrania

#define ETHER_TYPE  (0x0806) //EtherType hodnota pre ARP
#define OPCODE_REQ  (1) // ARP ZIADOST
#define OPCODE_RESP (2) // ARP ODPOVED
#define HW_LEN	    (6) // MAC adresa = 6B
#define IP_LEN      (4) // IP adresa = 4B
#define IP_PROTO    (0x0800) // IP
#define HW_TYPE     (0x0001) // Ethernet

struct ethHdr{
	uint8_t  dstMAC[6];
	uint8_t  srcMAC[6];
	uint16_t ethertype;
	uint8_t  payload[0]; // len formalne s 0-velkostou
} __attribute__ ((packed));

struct arpHdr{
	uint16_t hwType;
	uint16_t protoType;
	uint8_t  hwLen;
	uint8_t  protoLen;
	uint16_t opcode;
	uint8_t  srcMAC[6];
	uint32_t srcIP;
	uint8_t  targetMAC[6];
	uint32_t targetIP;
} __attribute__ ((packed));

int arpRequestAndResponse(char * dstIP){
	int sockClient;
	struct sockaddr_ll cAddr; 
	
	struct ethHdr *eth;
	struct arpHdr *arp;
	struct ethHdr *response;
	
	int i;
	
// ----- vytvorenie a zalozenie soketu ------

	if ((sockClient = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1){
		perror("socket()");
		return ERROR;
	}
	
	memset(&cAddr, 0, sizeof(cAddr));
	cAddr.sll_family = AF_PACKET;
	cAddr.sll_protocol = htons(ETH_P_ALL);
	
	if ((cAddr.sll_ifindex = if_nametoindex(IFACE)) == 0){
		perror("if_nametoindex()");
		close(sockClient);
		return ERROR;
	}

	if (bind(sockClient, (struct sockaddr *)&cAddr, sizeof(cAddr)) == -1){
		perror("bind()");
		close(sockClient);
		return ERROR;
	}

// ----- generovanie arp ziadosti -----	

	unsigned int msgLen = sizeof(struct ethHdr) + sizeof(struct arpHdr);
	
	if (msgLen < 60){
		msgLen = 60;
	} 
	
	uint8_t * msg = (uint8_t *) malloc(msgLen);
	if (msg == NULL) {
		perror("malloc()");
		close(sockClient);
		return ERROR;
	}
	
	memset(msg, 0, msgLen);

	eth = (struct ethHdr *) msg;	
	memset(eth->dstMAC, 0xFF, 6);
	
	sscanf(SRC_MAC,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &eth->srcMAC[0], &eth->srcMAC[1], &eth->srcMAC[2], &eth->srcMAC[3], &eth->srcMAC[4], &eth->srcMAC[5]);
	
	eth->ethertype = htons(ETHER_TYPE);
	
	arp = (struct arpHdr *) eth->payload;
	arp->hwType = htons(HW_TYPE);
	arp->protoType = htons(IP_PROTO);
	arp->hwLen = HW_LEN;
	arp->protoLen = IP_LEN;
	arp->opcode = htons(OPCODE_REQ);		
	
	for (i = 5; i >= 0; i--){
		arp->srcMAC[i] = eth->srcMAC[i];
	}

	struct in_addr ipAddr;

	memset(&ipAddr, 0, sizeof(struct in_addr));
	if (inet_aton(SRC_IP, &ipAddr) == 0){
		fprintf(stderr, "inet_aton(): Cannot convert text to IPv4 address.\n");
		close(sockClient);
		free((void *) msg);
		exit(ERROR);
	} else {
		arp->srcIP = ipAddr.s_addr;
	}

	memset(&ipAddr, 0, sizeof(struct in_addr));
	if (inet_aton(dstIP, &ipAddr) == 0){
		fprintf(stderr, "inet_aton(): Cannot convert text to IPv4 address.\n");
		close(sockClient);
		free((void *) msg);
		exit(ERROR);
	} else {
		arp->targetIP = ipAddr.s_addr;
	}
	
	if (write(sockClient, msg, msgLen) == -1){
		perror("write()");
		close(sockClient);
		free((void *) msg);
		return ERROR;
	}
	
// ------ cakanie na odpoved -------

	response = (struct ethHdr *) malloc(sizeof(struct ethHdr) + sizeof(struct arpHdr));
	
	if (response == NULL){
		perror("malloc()");
		close(sockClient);
		free((void *) msg);
		return ERROR;	
	}
	
	for (;;){
		struct arpHdr * arp_resp = (struct arpHdr *) response->payload;
				
		memset(response, 0, sizeof(struct ethHdr) + sizeof(struct arpHdr));
		read(sockClient, response, sizeof(struct ethHdr) + sizeof(struct arpHdr));
		
		if (response->ethertype != htons(ETHER_TYPE)){
			continue;
		}

		if (arp_resp->opcode != htons(OPCODE_RESP)){
			continue;
		}
		
		if (memcmp (&arp_resp->srcIP, &arp->targetIP, 4) != 0){
			continue;
		}

		// sprava je ARP odpovedou na nasu ziadost => splnila vsetky pozadovane podmienky

		struct in_addr src_ip;
		src_ip.s_addr = arp_resp->srcIP;			

		/*printf("Response from %s at %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
			inet_ntoa(src_ip),
			*(arp_resp->srcMAC),
			*(arp_resp->srcMAC+1),
			*(arp_resp->srcMAC+2),
			*(arp_resp->srcMAC+3),
			*(arp_resp->srcMAC+4),
			*(arp_resp->srcMAC+5));*/
			
		printf("Response from %s at %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
			inet_ntoa(src_ip),
			arp_resp->srcMAC[0],
			arp_resp->srcMAC[1],
			arp_resp->srcMAC[2],
			arp_resp->srcMAC[3],
			arp_resp->srcMAC[4],
			arp_resp->srcMAC[5]);
			
		break;
	}

	close(sockClient);
	free((void *) msg);
	free((void *) response);
	return SUCCESS;
}

int main(int argc, char * argv[]){
	int retValue;
	printf("START \n------\n\n");
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <destIP>\n", argv[0]);
		retValue = ERROR;	
	} else 	{	
		retValue = arpRequestAndResponse(argv[1]);
	}
	printf("\n------\nEND: [%s]\n", (retValue == SUCCESS)?"SUCCESS":"ERROR");
	return retValue; 
}
