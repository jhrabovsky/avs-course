
#ifndef BASIC_LIBS_H
#define BASIC_LIBS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <net/if.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#define		EXIT_ERROR	(1)
#define 	EXIT_SUCCESS (0)
#define		MACSIZE		(6)
#define		MTU (1500)

/*
 * Struktura IntDescriptor uchovava informacie o sietovom rozhrani, ktore
 * nas bridge obsluhuje - meno rozhrania, jeho cislo v Linuxe, a socket,
 * ktorym na tomto rozhrani citame a odosielame ramce.
 *
 */

struct IntDescriptor
{
	char name[IFNAMSIZ];
	unsigned int intNo;
	int socket;
	int sockpair[2]; // socketpair pre komunikaciu Reader<->Writer;
};

/*
 * Struktura MACAddress obaluje 6-bajtove pole pre uchovavanie MAC adresy.
 * Obalenie do struktury je vyhodne pri kopirovani (priradovani) MAC adresy
 * medzi premennymi rovnakeho typu.
 *
 */

struct MACAddress
{
	unsigned char MAC[MACSIZE];
} __attribute__ ((packed));

/*
 * Struct EthFrame reprezentuje zakladny ramec podla IEEE 802.3 s maximalnou
 * velkostou tela.
 *
 */

struct EthFrame
{
	struct MACAddress dest;
	struct MACAddress src;
	unsigned short type;
	char payload[MTU];
} __attribute__ ((packed));

#endif
