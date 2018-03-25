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

#define ERROR	(1)
#define SUCCESS	(0)

#define MSIZE 		(1472) // 1518 - 18 (L2=Ethernet II) - 20 (L3=IP) - 8 (L4=UDP)
#define DSTPORT 	(9999)

int main(int argc, char **argv)
{
	int sock;
	char message[MSIZE];
	struct sockaddr_in sockAddr;
	socklen_t addrLen;

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(ERROR);
	}


	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(DSTPORT);
	sockAddr.sin_addr.s_addr = INADDR_ANY; //vsetky rozhrania v systeme


	if(bind(sock, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) == -1)
	{
		perror("bind");
		close(sock);
		exit(ERROR);
	}

	printf("UDP message server \n");

	while(1)
	{
		addrLen = sizeof(sockAddr);
		if(recvfrom(sock, &message, MSIZE, 0, (struct sockaddr *)&sockAddr, &addrLen) == -1)
		{
			perror("recvfrom");
			break;
		}

		printf("Message from [%s:%d]: %s",
				inet_ntoa(sockAddr.sin_addr),
				ntohs(sockAddr.sin_port),
				message);
	}

	close(sock);
	return SUCCESS;
}


