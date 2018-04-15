#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	SERVER  "127.0.0.1"
#define	PORT		6050
#define	MLEN		80

int main(void)
{
  struct sockaddr_in Addr;
  int Socket;
  char Message[MLEN];

  if((Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  Addr.sin_family = AF_INET;
  Addr.sin_port = htons(PORT);
  if(inet_aton (SERVER, &Addr.sin_addr) == 0)
  {
    printf("inet_aton: Error in conversion\n");
    close(Socket);
    exit(1);
  }

  if(connect(Socket, (struct sockaddr *)&Addr, sizeof(Addr)) == -1)
  {
    perror("connect");
    close(Socket);
    exit (1);
  }

  memset (Message, '\0', MLEN);
  fgets (Message, MLEN, stdin);

  if(write(Socket, Message, strlen(Message)+1) == -1)
  {
      perror("write");
      close (Socket);
      exit(1);
  }

  close (Socket);
  return 0;
}
