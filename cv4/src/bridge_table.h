
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "basic_libs.h"

/*
 * Struct BTEntry je prvok zretazeneho zoznamu, ktory reprezentuje riadok
 * prepinacej tabulky.  V riadku je okrem smernikov na dalsi a predosly
 * prvok ulozena MAC adresa a smernik na rozhranie, kde je pripojeny klient,
 * ako aj cas, kedy sme videli tohto odosielatela naposledy.
 *
 */

struct BTEntry
{
	struct BTEntry *next;
	struct BTEntry *previous;
	struct MACAddress address;
	time_t lastSeen;
	struct IntDescriptor *IFD;
};

struct BTEntry * CreateBTEntry (void);
struct BTEntry * InsertBTEntry (struct BTEntry *Head, struct BTEntry *Entry);
struct BTEntry * AppendBTEntry (struct BTEntry *Head, struct BTEntry *Entry);
struct BTEntry * FindBTEntryByMAC (struct BTEntry *Head, const struct MACAddress *Address);
struct BTEntry * EjectBTEntryByItem(struct BTEntry *Head, struct BTEntry *Item);
struct BTEntry * EjectBTEntryByMAC (struct BTEntry *Head, const struct MACAddress *Address);
void DestroyBTEntry (struct BTEntry *Head, const struct MACAddress *Address);
void PrintBT (const struct BTEntry *Head);
struct BTEntry * FlushBT (struct BTEntry *Head);
struct BTEntry * UpdateOrAddMACEntry (struct BTEntry *Table, const struct MACAddress *Address, const struct IntDescriptor *IFD);

#endif
