#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>

#define		ERR_SYNTAX	1
#define		ERR_RUNTIME	2

#define		TEXTADDRLEN	40 // IPv4 aj IPv6
#define		BUFLEN		1460 // 1500 - 20 (IP) - 20 (TCP)

int main (int argc, char * argv[]) {
  int cliSock;
  int GAIResult;
  short int dstPort;
  const char * NToPResult = NULL;
  char addressAsText[TEXTADDRLEN];
  char buffer[BUFLEN];
  struct addrinfo * addressInfo = NULL;
  struct addrinfo addressHints;
  
  /*
   * Program ma jediny argument - meno servera.
   * Ak pocet argumentov nie je presne 2, t. j.
   * program a meno servera, oznamime chybu.
   */
   
  if (argc != 2) {
    fprintf (stderr, "Usage: %s <Server Name>\n\n", argv[0]);
    exit (ERR_SYNTAX);
  }
  
  /*
   * HTTP je sluzba nad TCP. Preto davame hint:
   * socket moze byt IPv4 alebo IPv6 (AF_UNSPEC),
   * typ socketu musi vsak byt TCP (SOCK_STREAM),
   * podprotokoly TCP nema (0), ziadne extra flagy (0).
   */

  addressHints.ai_family = AF_UNSPEC;
  addressHints.ai_socktype = SOCK_STREAM;
  addressHints.ai_protocol = 0;
  addressHints.ai_flags = 0;

  if ((GAIResult = getaddrinfo (argv[1], "http", &addressHints, &addressInfo)) != 0) {
    if (GAIResult == EAI_SYSTEM)
      perror ("getaddrinfo");
    else
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (GAIResult));

    exit (ERR_RUNTIME); 
  } // getaddrinfo() skoncil chybou
  
  /*
   * V tomto momente mame v &addressInfo pripravene
   * vysledky - aspon jeden. Zoberme prvy a pouzime.
   * Najprv vypiseme informacie o zistenej cielovej
   * IP a porte - musime rozlisit IPv4 a IPv6.
   */
   
  memset (addressAsText, '\0', TEXTADDRLEN);
  
  switch (addressInfo->ai_family) {
    case AF_INET:
      NToPResult = inet_ntop (AF_INET,
                 & ((struct sockaddr_in *) addressInfo->ai_addr)->sin_addr,
                 addressAsText, TEXTADDRLEN-1);
      dstPort = ntohs (((struct sockaddr_in *) addressInfo->ai_addr)->sin_port);
      break;
    
    case AF_INET6:
      NToPResult = inet_ntop (AF_INET6,
                 & ((struct sockaddr_in6 *) addressInfo->ai_addr)->sin6_addr,
                 addressAsText, TEXTADDRLEN-1);
      dstPort = ntohs (((struct sockaddr_in6 *) addressInfo->ai_addr)->sin6_port);        
      break;
  }
  
  if (NToPResult == NULL) { // ak inet_ntop() vratil chybu
    perror ("inet_ntop");
    freeaddrinfo (addressInfo);
    exit (ERR_RUNTIME);
  }
  
  printf ("Connecting to %s, port %d...\n",
          addressAsText, dstPort);
  
  /*
   * Vytvorime socket podla zodpovedajuceho typu
   * a skusime sa na druhu stranu pripojit.
   */

  if ((cliSock = socket (addressInfo->ai_family,
                              addressInfo->ai_socktype,
                              addressInfo->ai_protocol)) == -1) {
    // ak socket() vratil chybu

    perror ("socket");
    freeaddrinfo (addressInfo);
    exit (ERR_RUNTIME);
  }
  
  if (connect(cliSock,
              addressInfo->ai_addr,
              addressInfo->ai_addrlen) == -1) { // ak connect() vratil chybu

    perror ("connect");
    freeaddrinfo (addressInfo);
    exit (ERR_RUNTIME);
  }
  
  /*
   * Sme pripojeni. Skonstruujme ziadost a odoslime.
   */
   
  memset (buffer, '\0', BUFLEN);
  strcpy (buffer, "GET / HTTP/1.0\r\n\r\n");
  write (cliSock, buffer, strlen(buffer));
  
  /*
   * Nacitavajme, pokym druha strana posiela odpoved.
   */
  
  memset (buffer, '\0', BUFLEN);
  while (read (cliSock, buffer, BUFLEN) > 0) {
    fputs (buffer, stdout);
    memset (buffer, '\0', BUFLEN);
  }
  
  /*
   * Druha strana skoncila. Zavrieme spojenie
   * a skoncime program.
   */

  close (cliSock);
  freeaddrinfo (addressInfo);

  return 0;
}
