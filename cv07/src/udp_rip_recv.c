
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//====================================

#define MAX_MSG_LEN	(1472) // 1518 - 18 (L2=Ethernet II) - 20 (L3=IP) - 8 (L4=UDP)
#define MAX_IP_STR_LEN	(16)

#define RIP_MCAST_IP_STR	"224.0.0.9"
#define RIP_V2_PORT	(520)
#define RIP_CMD_RESPONSE (0x02)
#define RIP_VERSION	(0x02)

#define VALID	(1)
#define INVALID	(0)

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"

//====================================

struct rip_route_entry {
	uint16_t AF_id;
	uint16_t routeTag;
	struct in_addr netPrefix;
	struct in_addr mask;
	struct in_addr nextHop;
	uint32_t metric;
} __attribute__((packed));

struct rip_msg{
	uint8_t cmd;
	uint8_t version;
	uint16_t unused;
	struct rip_route_entry firstEntry[0];
} __attribute__((packed));

//====================================

int openUDP(void){
	int fd;
	struct sockaddr_in srcAddr;
	struct ip_mreqn mcastGrp;

	memset(&mcastGrp, 0, sizeof(struct ip_mreqn));
	mcastGrp.imr_address.s_addr = INADDR_ANY;
	mcastGrp.imr_ifindex = 0;
	if (inet_pton(AF_INET, RIP_MCAST_IP_STR, &(mcastGrp.imr_multiaddr)) == 0){
		fprintf(stderr, "inet_pton()\n");
		return -1;
	}

	memset(&srcAddr, 0, sizeof(struct sockaddr_in));
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons(RIP_V2_PORT);
	srcAddr.sin_addr.s_addr = INADDR_ANY;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("socket()");
		return -1;
	}

	if (bind(fd, (struct sockaddr *) &srcAddr, sizeof(struct sockaddr_in)) == -1){
		perror("bind()");
		close(fd);
		return -1;
	}

	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcastGrp, sizeof(struct ip_mreqn)) == -1){
		perror("setsockopt(IP_ADD_MEMBERSHIP)");
		close(fd);
		return -1;
	}

	return fd;
}

ssize_t recvMsg(int fd, char * srcIP, uint16_t * srcPort, uint8_t * msg){
	ssize_t msgLen;
	struct sockaddr_in sender;
	socklen_t addrLen = sizeof(struct sockaddr_in);

	memset(&sender, 0, sizeof(struct sockaddr_in));

	msgLen = recvfrom(fd, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&sender, &addrLen);
	strncpy(srcIP, inet_ntop(AF_INET, &sender.sin_addr, srcIP, MAX_IP_STR_LEN), MAX_IP_STR_LEN);
	*srcPort = ntohs(sender.sin_port);

	return msgLen;
}

ssize_t sendMsg(int fd, const char * dstIP, const short dstPort, const char * msg, int len){
	struct sockaddr_in dstAddr;
	ssize_t msgLen;

	memset(&dstAddr, 0, sizeof(struct sockaddr_in));
	dstAddr.sin_port = htons(dstPort);
	dstAddr.sin_family = AF_INET;

	if (inet_pton(AF_INET, dstIP, &(dstAddr.sin_addr)) == 0){
		fprintf(stderr, "Error: inet_aton() - cannot translate dstIP\n");
		return -1;
	}

	msgLen = sendto(fd, (void *) msg, len, 0, (struct sockaddr *)&dstAddr, sizeof(struct sockaddr_in));

	return msgLen;
}

/*
 * process all messages received from RIP mcast address (224.0.0.9)
 */

void printRIPMsg(uint8_t * rawMsg, ssize_t msgLen, char * srcIP, short srcPort){
	struct rip_msg * msg;
	struct rip_route_entry * entry;
	ssize_t procLen = 0;

	msg = (struct rip_msg *) rawMsg;

	printf(KGRN "RIP message [CMD=0x%02hhx VERSION:0x%02hhx] from %s:\n" KNRM, msg->cmd, msg->version, srcIP);
	procLen += sizeof(struct rip_msg);
	entry = msg->firstEntry;

	while (procLen < msgLen) {
		if (htons(entry->AF_id) == AF_INET){
			printf("\tprefix=%s", inet_ntoa(entry->netPrefix));
			printf("\tmaska=%s", inet_ntoa(entry->mask));
			printf("\tnext_hop=%s", inet_ntoa(entry->nextHop));
			printf("\tmetrika=%u\t", ntohl(entry->metric));
			printf("\ttag=%hu\n", ntohs(entry->routeTag));
			fflush(stdout);
		}

		procLen += sizeof (struct rip_route_entry);
		entry++;
	}
}

int checkRIPValidity(uint8_t * rawMsg, ssize_t msgLen){
	struct rip_msg * msg = (struct rip_msg *) rawMsg;

	if (msgLen < sizeof(struct rip_msg) + sizeof(struct rip_route_entry)){
		fprintf(stderr, KRED "Received RIP message does not have any route entry\n" KNRM);
		return INVALID;
	}

	// check integer count of entries in message
	if ((msgLen - sizeof(struct rip_msg)) % sizeof (struct rip_route_entry) != 0){
		fprintf(stderr, KRED "Received RIP message does not have integer count of entries\n" KNRM);
		return INVALID;
	}

	if (msg->cmd != RIP_CMD_RESPONSE){
		return INVALID;
	}

	return VALID;
}

void processRIP(int fd){
	uint8_t rawMsg[MAX_MSG_LEN];
	ssize_t msgLen;
	char srcIP[MAX_IP_STR_LEN];
	unsigned short srcPort;

	for (;;){
		// clear values / set them to zero
		memset(rawMsg, 0, sizeof(rawMsg));
		memset(srcIP, '\0', sizeof(srcIP));
		srcPort = 0;

		// receive msg from UDP/520 (RIP)
		msgLen = recvMsg(fd, srcIP, &srcPort, rawMsg);

		// check and process received RIP msg
		if (checkRIPValidity(rawMsg, msgLen) == VALID){
			printRIPMsg(rawMsg, msgLen, srcIP, srcPort);
		} else {
			printf(KRED "Received RIP message from %s:%hu is not a valid response\n" KNRM, srcIP, srcPort);
		}
	}
}

int main(int argc, char ** argv){
	int fd;
	fd = openUDP();

	if (fd<0){
		exit(EXIT_FAILURE);
	}

	for (;;){
		processRIP(fd);
	}

	close(fd);
	exit(EXIT_SUCCESS);
}
