#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h>

#define HOST "www.kis.fri.uniza.sk"

int main(void){
	struct addrinfo hints;
	struct addrinfo * hosts, * iter;
	char IPaddr[INET6_ADDRSTRLEN];
	struct in_addr ip4addr;
	struct in6_addr ip6addr;

	int code;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	code = getaddrinfo(HOST, NULL, &hints, &hosts);
	if (code != 0){
		fprintf(stderr, "getaddrinfo() : %s.\n", gai_strerror(code));
		return EXIT_FAILURE;
	}

	iter = hosts;
	while(iter != NULL){

		printf("%s:%s - ",
			(iter->ai_family == AF_INET)?"IPv4":"IPv6",
			(iter->ai_socktype == SOCK_STREAM)?"TCP":(iter->ai_socktype == SOCK_DGRAM)?"UDP":"Unknown");

		memset(IPaddr, 0, sizeof(IPaddr));
		if (iter->ai_family == AF_INET){
			ip4addr = ((struct sockaddr_in *) iter->ai_addr)->sin_addr;
			if (inet_ntop(AF_INET, (const void *) &ip4addr, IPaddr, sizeof(IPaddr)) == NULL){
				perror("inet_ntop()");
				return EXIT_FAILURE;
			}

		} else if (iter->ai_family == AF_INET6){
			ip6addr = ((struct sockaddr_in6 *) iter->ai_addr)->sin6_addr;
			if (inet_ntop(AF_INET6, (const void *) &ip6addr, IPaddr, sizeof(IPaddr)) == NULL){
				perror("inet_ntop()");
				return EXIT_FAILURE;
			}

		}
		printf("%s\n", IPaddr);

		iter = iter->ai_next;
	}

	freeaddrinfo(hosts);

	return EXIT_SUCCESS;
}
