
#include <pthread.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "basic_libs.h"
#include "bridge_table.h"

#define   MAXINTERFACES (8)
#define   MACMAXAGE (300)

/*
 * Globalne premenne - prepinacia tabulka, pole popisovacov rozhrani,
 * read/write zamok pre pracu nad tabulkou a pocet obsluhovanych rozhrani.
 *
 */

struct BTEntry *table = NULL;
struct IntDescriptor ints[MAXINTERFACES];
pthread_rwlock_t tableLock = PTHREAD_RWLOCK_INITIALIZER;
int intCount = 0;

/*
 * Telo vlakna, ktore sa kazdu sekundu zobudi, prebehne prepinaciu
 * tabulku a odstrani polozky, ktore su starsie ako MACMAXAGE sekund.
 *
 */

void *
DeleteUnusedMACThread (void *Arg){
  return NULL;
}

/*
 * Telo vlakna, ktore sa stara o ucenie sa alebo obnovovanie zdrojovych MAC.
 *
 */

void *
AddRefreshMACThread (void *Arg){
  return NULL;
}

/*
 * Telo vlakna, ktore sa stara o prepinanie ramcov podla prepinacej tabulky.
 *
 */

void *
FrameReaderThread (void *Arg){
  return NULL;
}

void initInterface(int index, const char * iface){

    struct ifreq ifr;
      struct sockaddr_ll addr;

      /* Prekopirujeme meno rozhrania do popisovaca rozhrania. */
      strncpy (ints[index].name, iface, IFNAMSIZ-1);

      /* Konvertujeme meno rozhrania na index a ulozime do popisovaca. */
      ints[index].intNo = if_nametoindex (iface);
      if (ints[index].intNo == 0)
  {
    perror ("if_nametoindex()");
    exit (EXIT_ERROR);
  }

      /* Pre dane rozhranie vyplnime strukturu sockaddr_ll potrebnu
         pre bind() */
      memset (&addr, 0, sizeof (struct sockaddr_ll));
      addr.sll_family = AF_PACKET;
      addr.sll_protocol = htons (ETH_P_ALL);
      addr.sll_ifindex = ints[index].intNo;

      /* Vytvorime socket typu AF_PACKET. */
      ints[index].socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
      if (ints[index].socket == -1)
  {
    perror ("socket()");
    /* TODO: Zavriet predchadzajuce sockety */
    exit (EXIT_ERROR);
  }

      /* Zviazeme socket s rozhranim. */
      if (bind
    (ints[index].socket, (struct sockaddr *) &addr,
     sizeof (struct sockaddr_ll)) == -1)
  {
    perror ("bind()");
    /* TODO: Zavriet predchadzajuce sockety */
    exit (EXIT_ERROR);
  }

      /* Priprava pre promiskuitny rezim.  Najprv ziskame sucasne priznaky
         rozhrania. */
      memset (&ifr, 0, sizeof (struct ifreq));
      strncpy (ifr.ifr_name, iface, sizeof(ifr.ifr_name));
      if (ioctl (ints[index].socket, SIOCGIFFLAGS, &ifr) == -1)
  {
    perror ("ioctl() - get flags");
    exit (EXIT_ERROR);
  }

      /* K priznakom pridame priznak IFF_PROMISC. */
      ifr.ifr_flags |= IFF_PROMISC;

      /* Nastavime nove priznaky rozhrania. */
      if (ioctl (ints[index].socket, SIOCSIFFLAGS, &ifr) == -1)
  {
    perror ("ioctl() - set flags");
    exit (EXIT_ERROR);
  }

      if (socketpair (AF_UNIX, SOCK_DGRAM, 0, ints[index].sockpair) == -1)
  {
    perror ("socketpair()");
    /* TODO: Upratat. */
    exit (EXIT_ERROR);
  }
}


int
main (int argc, char *argv[]){

  pthread_t threadID;

  /* Kontrola poctu argumentov pri spusteni programu. */
  if ((argc > MAXINTERFACES) || (argc == 1)){
      fprintf (stderr, "Usage: %s IF1 IF2 ... IF%d\n\n",
         argv[0], MAXINTERFACES);

      exit (EXIT_ERROR);
    }

  intCount = argc - 1;

    /* Inicializacia pola s popisovacmi rozhrani. */
  memset (ints, 0, sizeof (ints));

  for (int i = 1; i < argc; i++){
      initInterface(i-1, argv[i]);
    }

  /* Vytvorime prepinaciu tabulku zalozenim jej prveho riadku. */
  table = CreateBTEntry ();
  if (table == NULL){
      perror ("malloc");
      /* TODO: Zavriet sockety */
      exit (EXIT_ERROR);
    }

  pthread_create (&threadID, NULL, AddRefreshMACThread, NULL);
  pthread_create (&threadID, NULL, DeleteUnusedMACThread, NULL);

  for (int i = 0; i < intCount; i++){
      pthread_create (&threadID, NULL, FrameReaderThread, &(ints[i]));
    }

  (void) getchar();

  return 0;
}
