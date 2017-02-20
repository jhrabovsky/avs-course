#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h> // standardne typy => uint8_t, uint16_t, uint32_t, ...
#include <unistd.h> // close()
#include <string.h> // memset()

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include <arpa/inet.h>
#include <net/if.h>

#define ERROR	(0)
#define SUCCESS	(1)
#define IFACE	"eth0"
#define HELLO_TEXT	"Hello, how are you?"
#define ETH_TYPE	(0xDEAD)

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

int sendPacket(uint8_t * pkt, unsigned int pktSize, unsigned int count){
	int sock;
	struct sockaddr_ll addr; 

	if ((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1){
		perror("socket()");
		exit(ERROR);
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_ll));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ALL);
	
	if ((addr.sll_ifindex = if_nametoindex(IFACE)) == 0){
		perror("if_nametoindex()");
		close(sock);
		exit(ERROR);
	}

	if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_ll)) == -1){
		perror("bind()");
		close(sock);
		exit(ERROR);
	}
	
	for (int i = count; i > 0; i--){
		if (write(sock, pkt, pktSize) == -1){
			perror("write()");
			close(sock);
			exit(ERROR);
		}
	}

	close(sock);
	return SUCCESS;
}

int helloFrame(void){
	struct ethFrame msg;

	memset(&msg, 0, sizeof(msg));
	memset(msg.dstMAC, 0xFF, 6);
	msg.srcMAC[0] = 0x02;
	msg.srcMAC[5] = 0x13;
	msg.ethertype = htons(ETH_TYPE);	
	strncpy((char *) msg.payload, HELLO_TEXT, sizeof(msg.payload));
	
	return sendPacket((uint8_t *) &msg, sizeof(msg), 10);			
}

int main(void){
	int retValue;
	printf(" START \n-------\n");
	retValue = helloFrame();	
	printf("-------\nEND: [%s]\n", (retValue == SUCCESS) ? "SUCCESS" : "ERROR");
	return retValue; 
}

