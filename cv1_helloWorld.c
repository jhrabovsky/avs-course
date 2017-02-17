#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // standardne typy => uint8_t, uint16_t, uint32_t, ...
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
#define HELLO_TEXT	"Hello, how are you?"

struct ethFrame{
	uint8_t dstMAC[6];
	uint8_t srcMAC[6];
	uint16_t ethertype;
	uint8_t payload[1500];
} __attribute__ ((packed));

struct ethHdr{
	uint8_t dstMAC[6];
	uint8_t srcMAC[6];
	uint16_t ethertype;
} __attribute__ ((packed));

struct arpHdr{
	uint16_t hwType;
	uint16_t protoType;
	uint8_t hwLen;
	uint8_t protoLen;
	uint16_t opcode;
	uint8_t srcMAC[6];
	uint32_t srcIP;
	uint8_t targetMAC[6];
	uint32_t targetIP;
} __attribute__ ((packed));

int sendPacket(uint8_t * pkt, unsigned int pktSize, unsigned int count){
	int sockClient, i;
	struct sockaddr_ll cAddr; 

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
	
	for (i=count; i>0; i--){
		if (write(sockClient, pkt, pktSize) == -1){
			perror("write()");
			close(sockClient);
			return ERROR;
		}
	}

	close(sockClient);
	return SUCCESS;
}

int helloFrame(void){
	struct ethFrame msg;

	memset(&msg, 0, sizeof(msg));
	memset(msg.dstMAC, 0xFF, 6);
	msg.srcMAC[0] = 0x02;
	msg.srcMAC[5] = 0x01;
	msg.ethertype = htons(0xDEAD);	
	strcpy((char *)msg.payload, HELLO_TEXT);
	
	return sendPacket((uint8_t *) &msg, sizeof(msg), 2);			
}

int arpRequest_2(void){
	struct ethHdr *eth;
	struct arpHdr *arp;
	int ret;
	
	unsigned int msgLen = sizeof(struct ethHdr) + sizeof(struct arpHdr);
	
	if (msgLen < 60){
		msgLen = 60;
	} 
	
	uint8_t * msg = (uint8_t *) malloc(msgLen);
	if (msg == NULL) {
		perror("malloc()");
		return ERROR;
	}
	
	memset(msg, 0, msgLen);

	eth = (struct ethHdr *) msg;	
	memset(eth->dstMAC, 0xFF, 6);
	eth->srcMAC[0] = 0x02;
	eth->srcMAC[5] = 0x01;
	eth->ethertype = htons(0x806);
	
	arp = (struct arpHdr *) (msg + sizeof(struct ethHdr));
	arp->hwType = htons(1);
	arp->protoType = htons(0x800);
	arp->hwLen = 6;
	arp->protoLen = 4;
	arp->opcode = htons(1);		
	
	arp->srcMAC[0] = 0x02;
	arp->srcMAC[5] = 0x01;
	arp->srcIP = (uint32_t) inet_addr("192.168.1.3");
	arp->targetIP = (uint32_t) inet_addr("192.168.1.254");
	
	ret = sendPacket(msg, msgLen, 2);
	free((void *) msg);
	return ret;
}

int arpRequest(void){
	struct ethFrame msg;
	struct arpHdr * arp;

	memset(&msg, 0, sizeof(msg));
	memset(msg.dstMAC, 0xFF, 6);
	msg.srcMAC[0] = 0x02;
	msg.srcMAC[5] = 0x01;
	msg.ethertype = htons(0x806);
	
	arp = (struct arpHdr *) msg.payload;
	arp->hwType = htons(1);
	arp->protoType = htons(0x800);
	arp->hwLen = 6;
	arp->protoLen = 4;
	arp->opcode = htons(1);		
	
	arp->srcMAC[0] = 0x02;
	arp->srcMAC[5] = 0x01;
	arp->srcIP = (uint32_t) inet_addr("192.168.1.3");
	arp->targetIP = (uint32_t) inet_addr("192.168.1.254");
	
	return sendPacket((uint8_t *) &msg, sizeof(msg), 2);
}

int main(void){
	int retValue;
	printf(" START \n-------\n");
	//retValue = helloFrame();	
	//retValue = arpRequest();	
	retValue = arpRequest_2();	
	printf("-------\nEND: [%s]\n", (retValue == SUCCESS)?"SUCCESS":"ERROR");
	return retValue; 
}
