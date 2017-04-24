#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define		ERR_SUCCESS	 (0)
#define		ERR_NOTCREAT (1)
#define		ERR_EMPTY	 (2)

#define		EXIT_ERROR	 (1)

#define		IPCOUNT		(1000000)
#define		NETCOUNT	(20000)

#define PRINT_TABLE
#define PRINT_LOOKUP

struct RT {
    struct RTEntry * head;
    unsigned int count;
};

struct RTEntry {
    struct RTEntry *next;
    unsigned int address;
    unsigned int mask;
};

struct RT * CreateRTable(void) {
    struct RT * tab;

    tab = (struct RT *) malloc (sizeof (struct RT));
    if (tab == NULL){
        return NULL;
    }

    tab->head = NULL;
    tab->count = 0;

    return tab;
}

struct RTEntry * FindExisting (struct RT *Table, unsigned int Address, unsigned int Mask)
{
    struct RTEntry *I;

    if (Table == NULL){
        return NULL;
    }

    if (Table->head == NULL){
        return NULL;
    }

    I = Table->head;

    while (I != NULL){
        if ((I->address == Address) && (I->mask == Mask)){
            return I;
        }

        if (I->mask < Mask){
            return NULL;
        }

        I = I->next;
    }

    return NULL;
}

struct RTEntry * AddEntry (struct RT *Table, unsigned int Address, unsigned int Mask){
    struct RTEntry *I;
    struct RTEntry *Entry;

    if (Table == NULL){
        return NULL;
    }

    // overim, ze zadany prefix sa v tabulke este nenachadza
    if ((Entry = FindExisting (Table, Address, Mask)) != NULL){
        return Entry;
    }

    Entry = (struct RTEntry *) malloc (sizeof (struct RTEntry));
    if (Entry == NULL){
        return NULL;
    }

    Entry->address = Address;
    Entry->mask = Mask;

    if (Table->head == NULL){
        Table->head = Entry;
        Entry->next = NULL;
        Table->count += 1;
        return Entry;
    }

    I = Table->head;

    // presuniem sa na hranicny zaznam, tj dalsi zaznam v poradi uz ma mensiu masku
    while (I->next != NULL){
        if ((I->mask >= Mask) && (I->next->mask < Mask)){
            break;
        }

        I = I->next;
    }

    Entry->next = I->next;
    I->next = Entry;
    Table->count += 1;

    return Entry;
}

int PrintRTable (struct RT *Table){
    struct RTEntry *I;

    if (Table == NULL){
        return ERR_NOTCREAT;
    }

    printf("+-------------------+\n");
    printf("|   Routing Table   |\n");
    printf("+-------------------+\n\n");
    
    if (Table->head == NULL){
        return ERR_EMPTY;
    }

    I = Table->head;

    while (I != NULL){
        printf ("%hhu.%hhu.%hhu.%hhu/%hhu.%hhu.%hhu.%hhu\n",
            (I->address >> 24) & 0xFF,
            (I->address >> 16) & 0xFF,
            (I->address >> 8) & 0xFF,
            (I->address) & 0xFF,
            (I->mask >> 24) & 0xFF,
            (I->mask >> 16) & 0xFF,
            (I->mask >> 8) & 0xFF,
            (I->mask) & 0xFF);

        I = I->next;
    }

    return ERR_SUCCESS;
}

struct RTEntry * RoutingLookup (struct RT *Table, unsigned int DstAddr){
    struct RTEntry *I;

    if (Table == NULL){
        return NULL;
    }

    if (Table->head == NULL){
        return NULL;
    }

    I = Table->head;

    while (I != NULL){
        if ((DstAddr & (I->mask)) == I->address){
            return I;
        }

        I = I->next;
    }

    return NULL;
}

void FlushRTable (struct RT *Table){
    struct RTEntry *I, *J;

    if (Table == NULL){
        return;
    }

    if (Table->head == NULL){
        return;
    }

    I = Table->head;
    J = I->next;

    while (J != NULL){
        free(I);
        I = J;
        J = I->next;
    }

    free(I);
}

int GenerateNetworks (struct RT *Table, int Count){
    if (Table == NULL){
        return ERR_NOTCREAT;
    }

    srandom (time (NULL));

    for (int i = 0; i < Count; i++){
        int maskLen = ((float) random() / RAND_MAX) * 32; // random() % 33
        int mask = 0xFFFFFFFF << maskLen;
        int address = random() & mask;

        AddEntry (Table, address, mask);
    }

    return ERR_SUCCESS;
}

int main(void){
    struct RT *Table;
    unsigned int *IPs;
    struct timeval before, after, duration;
    int matchCount = 0;

    Table = CreateRTable();
    if (Table == NULL){
        perror ("CreateRTable");
        exit (EXIT_ERROR);
    }

    GenerateNetworks(Table, NETCOUNT);
    
#ifdef PRINT_TABLE
    PrintRTable(Table);
#endif

    IPs = (unsigned int *) malloc (IPCOUNT * sizeof (unsigned int));
    if (IPs == NULL){
        perror("malloc");
        FlushRTable(Table);
        free(Table);
        exit(EXIT_ERROR);
    }

    for (int i = 0; i < IPCOUNT; i++){
        IPs[i] = random();
    }

    printf("\n+-------------+\n");
    printf("|    LOOKUP   |\n");
    printf("+-------------+\n\n");
    
    fflush (stdout);
    gettimeofday (&before, NULL);

    for (int i = 0; i < IPCOUNT; i++){
        struct RTEntry *I = RoutingLookup(Table, IPs[i]);

#ifdef PRINT_LOOKUP
        printf ("%hhu.%hhu.%hhu.%hhu -> ",
            (IPs[i] >> 24) & 0xFF,
            (IPs[i] >> 16) & 0xFF,
            (IPs[i] >> 8) & 0xFF,
            (IPs[i]) & 0xFF);
#endif
        
        if (I != NULL){
            matchCount++;
            
#ifdef PRINT_LOOKUP        
            printf ("%hhu.%hhu.%hhu.%hhu/%hhu.%hhu.%hhu.%hhu\n",
            (I->address >> 24) & 0xFF,
            (I->address >> 16) & 0xFF,
            (I->address >> 8) & 0xFF,
            (I->address) & 0xFF,

            (I->mask >> 24) & 0xFF,
            (I->mask >> 16) & 0xFF,
            (I->mask >> 8) & 0xFF,
            (I->mask) & 0xFF);
#endif
        } else {

#ifdef PRINT_LOOKUP            
            printf("not found.\n");
#endif
        }
    }

    gettimeofday(&after, NULL);
    timersub(&after, &before, &duration);
    printf("DONE.\nTIME: %ld sec %ld usec, hits: %d, ratio: %f, avg lookup: %f usec/pkt, table size: %d\n",
        duration.tv_sec, duration.tv_usec, matchCount,
        ((float) matchCount) / IPCOUNT * 100,
        (float) (duration.tv_sec * 1000000 + duration.tv_usec) / IPCOUNT,
        Table->count);

    FlushRTable(Table);
    free(Table);

    exit(EXIT_SUCCESS);
}
