#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <net/if.h> // IFNAMSIZ (16)
#include <arpa/inet.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

// SELECT pre neblokujuci pristup k socketom
#include <sys/select.h>
#include <sys/ioctl.h>

// CASOVANIE => praca s casom
#include <time.h>

#define	MTU	(1500)
#define MACSIZE (6) // alternativa: pripojit <linux/if_ether.h> a pouzit ETH_ALEN
#define MAXINTERFACES (8)

#define ERROR	(1)
#define SUCCESS (0)

/*
 * Struktura IntDescriptor uchovava informacie o sietovom rozhrani, ktore
 * nas bridge obsluhuje - meno rozhrania, jeho cislo v Linuxe, a socket,
 * ktorym na tomto rozhrani citame a odosielame ramce.
 *
 */
 
struct IntDescriptor {
  char name[IFNAMSIZ];
  unsigned int intNo;
  int socket;
};

/*
 * Struktura MACAddress obaluje 6-bajtove pole pre uchovavanie MAC adresy. 
 * Obalenie do struktury je vyhodne pri kopirovani (priradovani) MAC adresy
 * medzi premennymi rovnakeho typu.
 *
 */

struct MACAddress {
  unsigned char MAC[MACSIZE];
} __attribute__ ((packed));

/*
 * Struct BTEntry je prvok zretazeneho zoznamu, ktory reprezentuje riadok
 * prepinacej tabulky.  V riadku je okrem smernikov na dalsi a predosly
 * prvok ulozena MAC adresa a smernik na rozhranie, kde je pripojeny klient,
 * ako aj cas, kedy sme videli tohto odosielatela naposledy.
 *
 */

struct BTEntry {
  struct BTEntry *next;
  struct BTEntry *previous;
  struct MACAddress address;
  time_t lastSeen;
  struct IntDescriptor *IFD;
};

/* 
 * Struct EthFrame reprezentuje zakladny ramec podla IEEE 802.3 s maximalnou
 * velkostou tela.
 *
 */

struct EthFrame {
  struct MACAddress dest;
  struct MACAddress src;
  unsigned short type;
  char payload[MTU];
} __attribute__ ((packed));

/*
 * Funkcia pre vytvorenie noveho riadku tabulky.  Riadok nebude zaradeny do
 * tabulky, bude mat len inicializovane vnutorne hodnoty.
 *
 */

struct BTEntry *
CreateBTEntry (void)
{
  struct BTEntry *E = (struct BTEntry *) malloc (sizeof (struct BTEntry));
  if (E != NULL)
    {
      memset (E, 0, sizeof (struct BTEntry));
      E->next = E->previous = NULL;
      E->IFD = NULL;
    }

  return E;
}

/*
 * Funkcia pre vlozenie vytvoreneho riadku do tabulky na jej ZACIATOK.
 *
 */

struct BTEntry *
InsertBTEntry (struct BTEntry *Head, struct BTEntry *Entry)
{
  if (Head == NULL)
    return NULL;

  if (Entry == NULL)
    return NULL;

  Entry->next = Head->next;
  Entry->previous = Head;
  Head->next = Entry;

  return Entry;
}

/*
 * Funkcia pre vlozenie vytvoreneho riadku do tabulky na jej koniec.
 *
 */

struct BTEntry *
AppendBTEntry (struct BTEntry *Head, struct BTEntry *Entry)
{
  struct BTEntry *I;

  if (Head == NULL)
    return NULL;

  if (Entry == NULL)
    return NULL;

  // dostan sa na posledny prvok v zozname
  I = Head;
  while (I->next != NULL)
    I = I->next;

  I->next = Entry;
  Entry->previous = I;

  return Entry;
}

/*
 * Funkcia hladajuca riadok tabulky podla zadanej MAC adresy.
 *
 */

struct BTEntry *
FindBTEntry (struct BTEntry *Head, const struct MACAddress *Address)
{
  struct BTEntry *I;

  if (Head == NULL)
    return NULL;

  if (Address == NULL)
    return NULL;

  I = Head->next;
  while (I != NULL){
      if (memcmp (&(I->address), Address, MACSIZE) == 0)
		return I;

      I = I->next;
    }

  return NULL;
}

/*
 * Funkcia, ktora z tabulky vysunie riadok, ak nanho uz mame smernik.
 * Riadok nebude dealokovany z pamate, bude len vynaty z tabulky.
 *
 */

struct BTEntry *
EjectBTEntryByItem (struct BTEntry *Head, struct BTEntry *Item)
{
  if (Head == NULL)
    return NULL;

  if (Item == NULL)
    return NULL;

  (Item->previous)->next = Item->next;
  if (Item->next != NULL)
    (Item->next)->previous = Item->previous;
    
  Item->next = Item->previous = NULL;

  return Item;
}

/*
 * Funkcia sluziaca na vynatie riadku z tabulky.  Riadok nebude dealokovany
 * z pamate, bude len odstraneny z tabulky.  Vyhladava sa podla MAC adresy.
 *
 */
 
struct BTEntry *
EjectBTEntryByMAC (struct BTEntry *Head, const struct MACAddress *Address)
{
  struct BTEntry *E;

  if (Head == NULL)
    return NULL;

  if (Address == NULL)
    return NULL;

  E = FindBTEntry (Head, Address);
  if (E == NULL)
    return NULL;
	
  E = EjectBTEntryByItem(Head, E);

  return E;
}

/*
 * Funkcia na vynatie a uplne zrusenie riadku tabulky i z pamate.  Vyhladava
 * sa podla MAC adresy.
 *
 */

void
DestroyBTEntry (struct BTEntry *Head, const struct MACAddress *Address)
{
  struct BTEntry *E;

  E = EjectBTEntryByMAC (Head, Address);
  if (E != NULL)
    free (E);

  return;
}

/*
 * Funkcia na vypis obsahu prepinacej tabulky.
 *
 */

void
PrintBT (const struct BTEntry *Head)
{

#define TLLEN (2+IFNAMSIZ+3+17+2+1)

  struct BTEntry *I;
  char TableLine[TLLEN];

  if (Head == NULL) {
	 printf("Table doesn't exist!\n");
	 return;
  }
  
  // Vypis HLAVICKY tab
  
  memset (TableLine, '-', TLLEN - 2);
  TableLine[0] = TableLine[2 + IFNAMSIZ + 1] = TableLine[TLLEN - 2] = '+';
  TableLine[TLLEN - 1] = '\0';
  printf (TableLine);
  printf ("\n");

  memset (TableLine, ' ', TLLEN - 2);
  TableLine[0] = '|';
  strncpy (TableLine + 2, "Interface", 9);
  TableLine[2 + IFNAMSIZ + 1] = '|';
  strncpy (TableLine + 2 + IFNAMSIZ + 1 + 2, "MAC Address", 11);
  TableLine[TLLEN - 2] = '|';
  printf (TableLine);
  printf ("\n");

  memset (TableLine, '-', TLLEN - 2);
  TableLine[0] = TableLine[2 + IFNAMSIZ + 1] = TableLine[TLLEN - 2] = '+';
  TableLine[TLLEN - 1] = '\0';
  printf (TableLine);
  printf ("\n");

  if (Head->next == NULL)
    return;

  I = Head->next;
  while (I != NULL)
    {
      memset (TableLine, ' ', TLLEN - 2);
      TableLine[0] = '|';

      strncpy (TableLine + 2, I->IFD->name, strlen (I->IFD->name));
      TableLine[2 + IFNAMSIZ + 1] = '|';
      sprintf (TableLine + 2 + IFNAMSIZ + 1 + 2,
	       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
	       I->address.MAC[0], I->address.MAC[1],
	       I->address.MAC[2], I->address.MAC[3],
	       I->address.MAC[4], I->address.MAC[5]);
      TableLine[TLLEN - 3] = ' ';
      TableLine[TLLEN - 2] = '|';

      printf (TableLine);
      printf ("\n");

      I = I->next;
    }

  // Vypis ukoncovacej ciary 
  memset (TableLine, '-', TLLEN - 2);
  TableLine[0] = TableLine[2 + IFNAMSIZ + 1] = TableLine[TLLEN - 2] = '+';
  TableLine[TLLEN - 1] = '\0';
  printf (TableLine);
  printf ("\n");

  return;
}

/*
 * Funkcia uplne zrusi a dealokuje z pamate celu prepinaciu tabulku, necha
 * iba pociatocny zaznam.
 *
 */

struct BTEntry *
FlushBT (struct BTEntry *Head)
{
  struct BTEntry *I;

  if (Head == NULL)
    return NULL;

  if (Head->next == NULL)
    return Head;

  I = Head->next;
  while (I->next != NULL)
    {
      I = I->next;
      free (I->previous);
    }

  free (I);

  Head->next = Head->previous = NULL;

  return Head;
}

/*
 * Funkcia realizujuca obsluhu zdrojovej adresy v prepinacej tabulke.  Ak
 * adresa v tabulke neexistuje, funkcia pre nu vytvori a do tabulky vlozi
 * novy zaznam.  Ak adresa v tabulke existuje, funkcia podla potreby
 * aktualizuje zaznam o vstupnom rozhrani, a v kazdom pripade aktualizuje
 * cas, kedy sme naposledy tuto zdrojovu adresu videli.
 *
 */

struct BTEntry *
UpdateOrAddMACEntry (struct BTEntry *Table, const struct MACAddress *Address,
		     const struct IntDescriptor *IFD)
{
  struct BTEntry *E;

  if (Table == NULL)
    return NULL;

  /* Vyhladame zdrojovu adresu v tabulke. */
  if ((E = FindBTEntry (Table, Address)) == NULL)
    {

      /* Adresa je neznama. Zalozime pre nu novy zaznam. */
      E = CreateBTEntry ();
      if (E == NULL){
		perror ("malloc");
		/* TODO: upratat */
		exit (ERROR);
	  }

      E->address = *Address;
      E->IFD = (struct IntDescriptor *) IFD;

      printf ("Adding address %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx to interface %s\n",
	      E->address.MAC[0],
	      E->address.MAC[1],
	      E->address.MAC[2],
	      E->address.MAC[3],
	      E->address.MAC[4],
	      E->address.MAC[5], E->IFD->name);
      InsertBTEntry (Table, E);
      PrintBT (Table);
    }
  else if (E->IFD != IFD) {
    /* Adresa je znama, ale naucena na inom rozhrani - aktualizujeme. */
    E->IFD = (struct IntDescriptor *) IFD;
  }
  
  E->lastSeen = time (NULL);

  return E;
}

int
main (int argc, char *argv[])
{
  struct IntDescriptor ints[MAXINTERFACES];
  int maxSockNo = 0;
  struct BTEntry *table = NULL;

  /* Kontrola poctu argumentov pri spusteni programu. */
  if ((argc > MAXINTERFACES) || (argc == 1)){
      fprintf (stderr, "Usage: %s IF1 IF2 ... IF%d\n\n", argv[0], MAXINTERFACES);
      exit (ERROR);
    }

  /* Inicializacia pola s popisovacmi rozhrani. */
  memset (ints, 0, sizeof(ints));

  /*
   * V cykle sa postupne pre kazde rozhranie vybavia tri klucove
   * zalezitosti: otvorime socket typu AF_PACKET pre vsetky ramce, zviazeme
   * ho s prislusnym rozhranim a rozhranie presunieme do promiskuitneho
   * rezimu.
   *
   */

  for (int i = 1; i < argc; i++){
	  
      struct ifreq IFR;
      struct sockaddr_ll SA;

      // Prekopirujeme meno rozhrania do popisovaca rozhrania
      strncpy(ints[i - 1].name, argv[i], IFNAMSIZ-1);

      // Konvertujeme meno rozhrania na index a ulozime do popisovaca
      ints[i - 1].intNo = if_nametoindex(argv[i]);
      if (ints[i - 1].intNo == 0){
		perror ("if_nametoindex");
		exit (ERROR);
	  }

      // Pre dane rozhranie vyplnime strukturu sockaddr_ll potrebnu pre bind()
      memset (&SA, 0, sizeof (struct sockaddr_ll));
      SA.sll_family = AF_PACKET;
      SA.sll_protocol = htons(ETH_P_ALL);
      SA.sll_ifindex = ints[i - 1].intNo;

      // Vytvorime socket typu AF_PACKET
      if ((ints[i - 1].socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) == -1){
		perror ("socket");
		/* TODO: Zavriet predchadzajuce sockety */
		exit (ERROR);  
	  }
	
      // Zviazeme socket s rozhranim
      if (bind(ints[i - 1].socket, (struct sockaddr *) &SA, sizeof (struct sockaddr_ll)) == -1){
		perror ("bind");
		/* TODO: Zavriet predchadzajuce sockety */
		exit (ERROR);
	  }

      // Priprava pre promiskuitny rezim => ziskame sucasne priznaky rozhrania
      memset(&IFR, 0, sizeof (struct ifreq));
      strncpy (IFR.ifr_name, ints[i - 1].name, IFNAMSIZ - 1);
      if (ioctl (ints[i - 1].socket, SIOCGIFFLAGS, &IFR) == -1){
		perror ("ioctl get flags");
		exit (ERROR);
	  }

      // K priznakom pridame priznak IFF_PROMISC
      IFR.ifr_flags |= IFF_PROMISC;

      // Nastavime nove priznaky rozhrania
      if (ioctl(ints[i - 1].socket, SIOCSIFFLAGS, &IFR) == -1){
		perror ("ioctl set flags");
		exit (ERROR);
	  }
	  
	  // Odkladame si maximalne cislo socketu pre neskorsi select()
      if (ints[i - 1].socket > maxSockNo){
		  maxSockNo = ints[i - 1].socket;
	  }
	  
    } // KONIEC INICIALIZACIE ROZHRANI

  // Najvyssie cislo socketu zvysime o 1, ako pozaduje select()
  maxSockNo += 1;

  // Vytvorime prepinaciu tabulku zalozenim jej prveho virtualneho riadku
  if ((table = CreateBTEntry ()) == NULL){
      perror ("malloc");
      /* TODO: Zavriet sockety */
      exit (ERROR);
  }

  /*
   * Hlavna cast programu.  V nekonecnej slucke program caka na udalost na
   * niektorom zo sledovanych socketov.  Po zisteni, ze nejaka udalost
   * nastala, program zisti, na ktorom sockete, potom z daneho socketu
   * nacita ramec, vyhlada a pripadne prida adresu odosielatela do tabulky,
   * vyhlada adresu prijemcu a odosle ramec - bud len danym rozhranim (nikdy
   * vsak nie vstupnym), alebo vsetkymi ostatnymi.
   *
   */
   
  for (;;){
      
	  fd_set FDs;

      /* Do mnoziny FDs pridame vsetky sockety. */
      FD_ZERO (&FDs);
      for (int i = 0; i < argc - 1; i++){
		FD_SET(ints[i].socket, &FDs);
	  }

      /* Nad tymito socketmi cakame na udalost prijatia ramca. */
      select (maxSockNo, &FDs, NULL, NULL, NULL);

      /* Ak udalost nastala, zistime, na ktorom sockete... */
      for (int i = 0; i < argc - 1; i++){
		if (FD_ISSET (ints[i].socket, &FDs)){
			/* ... a ideme ju obsluzit. */
			struct EthFrame frame;
			int frameLength;
			struct BTEntry *E;

			/* Nacitame ramec, maximalne do dlzky MTU. */
			frameLength = read(ints[i].socket, &frame, sizeof (struct EthFrame));

			/* Obsluzime zdrojovu adresu.  Bud ju uz pozname, a v takom
			pripade musime aktualizovat udaj o case, kedy sme ju
			naposledy pouzili (prave teraz), pripadne aj vstupny
			interfejs, alebo ju nepozname, a v takom pripade ju musime
			pridat.  */
			
			E = UpdateOrAddMACEntry (table, &(frame.src), &(ints[i]));

			/* Obsluzime cielovu adresu.  Ak ju nepozname, ramec rozosleme
			vsetkymi ostatnymi rozhraniami okrem vstupneho.  Ak ju
			pozname, ramec odosleme len danym rozhranim, ak je rozne od
			vstupneho.  */
	    
			if ((E = FindBTEntry (table, &(frame.dest))) == NULL){
				for (int j = 0; j < argc - 1; j++){
					if (i != j){
						write (ints[j].socket, &frame, frameLength);
					}
				}
			} else if (E->IFD != &(ints[i])){
				write (E->IFD->socket, &frame, frameLength);
			}
		}
    }
  } // NEKONECNA SLUCKA => OBSLUHA

  // TODO: upratat
  
  return SUCCESS;
}






















