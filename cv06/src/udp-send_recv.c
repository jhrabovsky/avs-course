#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <pthread.h>

#define ERROR	(1)
#define SUCCESS	(0)

#define MSIZE 		(1472) // 1518 - 18 (L2=Ethernet II) - 20 (L3=IP) - 8 (L4=UDP)
#define DSTPORT 	(9999)
#define DSTADDR 	"192.168.11.255"

void * receive_thread(void *arg)
{
	int sock = *((int *)arg);
	socklen_t addrLen;
	struct sockaddr_in receiveSockAddr;
	char receiveMessage[MSIZE];
	while(1)
	{
		addrLen = sizeof(receiveSockAddr);
		if(recvfrom(sock, &receiveMessage, MSIZE, 0, (struct sockaddr *)&receiveSockAddr, &addrLen) == -1)
		{
			perror("recvfrom");
			break;
		}

		printf("Message from [%s:%d]: %s",
				inet_ntoa(receiveSockAddr.sin_addr),
				ntohs(receiveSockAddr.sin_port),
				receiveMessage);
	}
}

int main(int argc, char **argv)
{
	int sock;
	char message[MSIZE];
	struct sockaddr_in sockAddr;
	
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(ERROR);
	}


	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(DSTPORT);
	if(inet_aton(DSTADDR, &sockAddr.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid destination Address\n");
		close(sock);
		exit(ERROR);
	}

	int allowBroadcast = 1;

	//povolenie posielania Broadcastu
	if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &allowBroadcast, sizeof(allowBroadcast)) == -1)
	{
		perror("setsockopt");
		close(sock);
		exit(ERROR);
	}

	if(bind(sock, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) == -1)
	{
		perror("bind");
		close(sock);
		exit(ERROR);
	}

	//vlakno na prijimanie sprav
	pthread_t receiveThreadPthread;
	pthread_create(&receiveThreadPthread, NULL, receive_thread, (void *) &sock);
	
	while(1)
	{
		memset(&message, '\0', MSIZE);
		printf("> "); fflush(stdout);
		fgets(message, MSIZE, stdin);

		if(sendto(sock, message, strlen(message)+1, 0,
				(struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1)
		{
			perror("sendto");
			break;
		}
	}

	close(sock);
	return SUCCESS;
}


