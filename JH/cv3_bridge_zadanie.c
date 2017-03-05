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

#include <time.h>

//==================== 
//		MAKRA	
//====================
 
#define	MTU	(1500)
#define MACSIZE (6)
#define MAXINTERFACES (8)

#define ERROR	(1)
#define SUCCESS (0)

/*
 * Struktura IntDescriptor uchovava informacie o sietovom rozhrani, ktore
 * nas bridge obsluhuje - meno rozhrania, jeho index v Linuxe a socket,
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
 * velkostou tela podla MTU.
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
CreateBTEntry (void){
  struct BTEntry *E = (struct BTEntry *) malloc (sizeof (struct BTEntry));
  if (E != NULL){
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

  // TODO: Vloz prvok na koniec zoznamu.

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
      if (memcmp (&(I->address), Address, MACSIZE) == 0){
		return I;
	  }
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

  // TODO: Najdi prvok v tabulke podla MAC adresy.
  
	// TODO: Ak existuje, vyhod ho z tabulky.

  return E;
}

/*
 * Funkcia na vynatie a uplne zrusenie riadku tabulky aj z pamate. Vyhladava
 * sa podla MAC adresy.
 *
 */

void
DestroyBTEntryByMAC (struct BTEntry *Head, const struct MACAddress *Address)
{
  struct BTEntry *E;

  E = EjectBTEntryByMAC(Head, Address);
  if (E != NULL){
    free((void *) E);
  }
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
  while (I->next != NULL){
      I = I->next;
      free (I->previous);
  }

  free (I);

  Head->next = Head->previous = NULL;

  return Head;
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
  // TODO: Over spravnost vstupnych argumentov.
  
  // TODO: Vyhladaj MAC adresu v tabulke.
  
	// TODO: Ak nasiel, aktualizuj hodnoty zaznamu.

	// TODO: Ak nenasiel, vytvor a pridaj novy zaznam.
  
  // TODO: Aktualizuj cas poslednej zmeny v zazname.
  
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
  memset(ints, 0, sizeof(ints));

  /*
   * V cykle sa postupne pre kazde rozhranie vybavia tri klucove
   * zalezitosti: otvorime socket typu AF_PACKET pre vsetky ramce, zviazeme
   * ho s prislusnym rozhranim a rozhranie presunieme do promiskuitneho
   * rezimu.
   *
   */

  for (int i = 1; i < argc; i++){
	  
	  // TODO: Nastav hodnoty popisovaca pre rozhranie.
	  
	  // TODO: Vytvor socket z domeny af_packet.
	  
	  // TODO: Nastav adresu.
	  
	  // TODO: Prepoj socket s adresou.
      
	  // TODO: Nastav promiskuitny rezim.
	  
    } 

  // Najvyssie cislo socketu zvysime o 1, ako pozaduje select()
  maxSockNo += 1;

  // Vytvorime prepinaciu tabulku zalozenim jej prveho virtualneho riadku
  if ((table = CreateBTEntry ()) == NULL){
      perror ("malloc");
      // TODO: Zavriet sockety.
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
			frameLength = read(ints[i].socket, &frame, sizeof(struct EthFrame));

			/* Obsluzime zdrojovu adresu.  Bud ju uz pozname, a v takom
			pripade musime aktualizovat udaj o case, kedy sme ju
			naposledy pouzili (prave teraz), pripadne aj vstupny
			interfejs, alebo ju nepozname, a v takom pripade ju musime
			pridat.  */
			
			UpdateOrAddMACEntry (table, &(frame.src), &(ints[i]));

			// TODO: Vyhladaj zaznam v tabulke, ktory prislucha cielovej MAC prijateho ramca.
			
			// TODO: Ak si nasiel a vystupne rozhranie je ine nez vstupne, posli ramec na vystupne rozhranie.
			
			// TODO: Ak si nenasiel, posli ramec na vsetky rozhrania okrem vstupneho.

		}
    }
  } // NEKONECNA SLUCKA => OBSLUHA

  // TODO: Uprac.
  
  return SUCCESS;
}






















