/*
 * Kompilovat prikazom gcc -std=gnu99 -Wall 01-helloworld.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include <net/if.h>

#define		EXIT_ERROR		(1)
#define		INTERFACE		"eth0"
#define		MYPROTO			(0xDEAD)
#define		MYMESSAGE		"Hello World from PeterP"
#define		FRAMECOUNT		(512)

/*
 * Format ramca: Cielova MAC, Zdrojova MAC, EtherType, Telo
 *
 * Kompilator nesmie medzi polozky struktury vlozit vyplnove bajty
 * na zarovnanie poloziek na vhodne adresy
 */

struct EthFrame
{
  unsigned char DstMAC[6];
  unsigned char SrcMAC[6];
  unsigned short EthTYPE;
  char Payload[1500];
} __attribute__ ((packed));

int
main (void)
{
  int Socket;
  struct sockaddr_ll Addr;
  struct EthFrame MyFrame;

  /*
   * Vytvorime socket adresovej rodiny AF_PACKET typu RAW
   */

  Socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL));

  if (Socket == -1)
    {
      perror ("socket");
      exit (EXIT_ERROR);
    }

  /*
   * Pripravime si obsah struktury struct sockaddr_ll s informaciami
   * pre lokalnu adresu socketu (v pripade AF_PACKET je to ID sietoveho
   * rozhrania, na ktorom sa budu ramce odosielat a prijimat)
   */

  memset (&Addr, 0, sizeof (Addr));
  Addr.sll_family = AF_PACKET;
  Addr.sll_protocol = htons (ETH_P_ALL);
  Addr.sll_ifindex = if_nametoindex (INTERFACE);
  if (Addr.sll_ifindex == 0)
    {
      perror ("if_nametoindex");
      close (Socket);
      exit (EXIT_ERROR);
    }

  /*
   * Zviazeme socket s lokalnou adresou v strukture sockaddr_ll
   */

  if (bind (Socket, (struct sockaddr *) &Addr, sizeof (Addr)) == -1)
    {
      perror ("bind");
      close (Socket);
      exit (EXIT_ERROR);
    }

  /*
   * Pripravime si ramec
   */

  memset (&MyFrame, 0, sizeof (MyFrame));
  memset (&MyFrame, 0xFF, 6);
  MyFrame.SrcMAC[0] = 2;
  MyFrame.EthTYPE = htons (MYPROTO);
  strcpy (MyFrame.Payload, MYMESSAGE);

  /*
   * V cykle ramec odosielame, pricom hodnotu premennej i zapisujeme
   * do spodnych 4B zdrojovej MAC adresy.
   */

  for (unsigned int i = 0; i < FRAMECOUNT; i++)
    {
      *((unsigned int *) (&(MyFrame.SrcMAC[2]))) = htonl (i);
      if (write (Socket, &MyFrame, sizeof (MyFrame)) == -1)
	{
	  perror ("write");
	  close (Socket);
	  exit (EXIT_ERROR);
	}
    }

  close (Socket);
  exit (EXIT_SUCCESS);
}
