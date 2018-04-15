#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	PORT	       6050
#define	MLEN		     80
#define	QUEUE_LENGTH 10

void HandleClient (int ClientSocket, struct sockaddr_in *ClientAddr)
{
  char Message[MLEN];

  memset(Message, '\0', MLEN);
  recv(ClientSocket, Message, MLEN, 0);

  printf("Message from [IP: %s, port: %d]: %s\n",
        inet_ntoa(ClientAddr->sin_addr),
        ntohs(ClientAddr->sin_port),
	      Message);

  close(ClientSocket);
  exit(0);
}

int main(void)
{
  struct sockaddr_in Addr, ClientAddr;
  int Socket, ClientSocket;
  int SockOption = 1;
  socklen_t SockLen;

  if ((Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  Addr.sin_family = AF_INET;
  Addr.sin_port = htons (PORT);
  Addr.sin_addr.s_addr = INADDR_ANY;

  if(setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, &SockOption, sizeof(SockOption)) == -1)
  {
    perror("setsockopt");
    close(Socket);
    exit(1);
  }

  if(bind(Socket, (struct sockaddr *)&Addr, sizeof(Addr)) == -1)
  {
    perror("bind");
    close(Socket);
    exit(1);
  }

  if(listen(Socket, QUEUE_LENGTH) == -1)
  {
    perror("listen");
    close(Socket);
    exit(1);
  }

  while(1)
  {
    SockLen = sizeof(ClientAddr);
    if((ClientSocket = accept(Socket, (struct sockaddr *)&ClientAddr, &SockLen)) == -1)
    {
      perror("accept");
      close(Socket);
      exit(1);
    }

    switch (fork ())
    {
      case -1:
        perror("fork");
        close(Socket);
        exit(1);
        break;

      case 0:
        HandleClient(ClientSocket, &ClientAddr);
        break;
    }
    close (ClientSocket);
  }
  
  close(Socket);
  return 0;
}
