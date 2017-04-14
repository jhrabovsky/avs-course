#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define		PORT		6050
#define		MLEN		80
#define		BACKLOG		10


void HandleZombie (int SigNo)
{
  wait (NULL);
}

void HandleClient (int ClientSocket)
{
  char Message[MLEN];
  time_t Time;
  
  memset (Message, '\0', MLEN);
  Time = time (NULL);
  strncpy (Message, ctime (&Time), MLEN-1);
  write (ClientSocket, Message, strlen(Message)+1);
  sleep (10);
  close (ClientSocket);
  
  exit (0);
}

int main(void)
{
  struct sockaddr_in Addr, ClientAddr;
  int Socket, ClientSocket;
  int SockOption = 1;
  socklen_t SockLen;
  struct sigaction SA;
  
  if (sigaction (SIGCHLD, NULL, &SA) == -1) {
    perror ("sigaction get");
    exit (1);
  }
  
  SA.sa_handler = HandleZombie;

  if (sigaction (SIGCHLD, &SA, NULL) == -1) {
    perror ("sigaction set");
    exit (1);
  }

  if ((Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror ("socket");
    exit (1);
  }
  
  Addr.sin_family = AF_INET;
  Addr.sin_port = htons (PORT);
  Addr.sin_addr.s_addr = INADDR_ANY;

  if (setsockopt (Socket, SOL_SOCKET, SO_REUSEADDR,
              &SockOption, sizeof(SockOption)) == -1) {
  
    perror ("setsockopt");
    exit (1);
  }

  if (bind (Socket, (struct sockaddr *) &Addr, sizeof(Addr)) == -1) {
    perror ("bind");
    exit (1);
  }
  
  listen (Socket, BACKLOG);
  
  while (1) {
    SockLen = sizeof(ClientAddr);
    ClientSocket = accept (Socket, (struct sockaddr *) &ClientAddr,
                           &SockLen);

    if (ClientSocket == -1) {
      if (errno == EINTR)
        continue;
      
      perror ("accept");
      exit (1);
    }
    
    switch (fork ()) {
      case -1:
        perror ("fork");
        exit (1);
        break;
      
      case 0:
        HandleClient (ClientSocket);
        break;
    }

    close (ClientSocket);
    
    fprintf (stdout, "Client connected, IP: %s, port: %d\n",
              inet_ntoa (ClientAddr.sin_addr),
              ntohs (ClientAddr.sin_port) );
    
  }
  
  return 0;

}
