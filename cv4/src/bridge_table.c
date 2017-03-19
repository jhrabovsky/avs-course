
#include "bridge_table.h"

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
 * Funkcia pre vlozenie vytvoreneho riadku do tabulky na jej zaciatok.
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
FindBTEntryByMAC (struct BTEntry *Head, const struct MACAddress *Address)
{
	struct BTEntry *I;

	if (Head == NULL)
		return NULL;

	if (Address == NULL)
		return NULL;

	I = Head->next;
	while (I != NULL)
	{
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
 * Funkcia, ktora z tabulky vysunie riadok so zadanou MAC adresou.
 * Riadok nebude dealokovany z pamate, bude len vynaty z tabulky.
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

	E = FindBTEntryByMAC (Head, Address);
	if (E == NULL)
		return NULL;

	E = EjectBTEntryByItem (Head, E);

	return E;
}


/*
 * Funkcia na vynatie a uplne zrusenie riadku tabulky i z pamate.  Vyhladava
 * sa podla MAC adresy.
 *
 */

void
DestroyBTEntryByMAC (struct BTEntry *Head, const struct MACAddress *Address)
{
	struct BTEntry *E;

	if (Head == NULL)
		return;

	if (Address == NULL)
		return;

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

	if (Head == NULL)
		return;

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
		return Head;

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
	if ((E = FindBTEntryByMAC (Table, Address)) == NULL)
	{
		/* Adresa je neznama. Zalozime pre nu novy zaznam. */
		E = CreateBTEntry ();
		if (E == NULL)
		{
			perror ("malloc");
			/* TODO: ... ;) */
			exit (EXIT_ERROR);
		}

		E->address = *Address;
		E->IFD = (struct IntDescriptor *) IFD;

		printf ("Adding address %x:%x:%x:%x:%x:%x to interface %s\n",
				E->address.MAC[0],
				E->address.MAC[1],
				E->address.MAC[2],
				E->address.MAC[3],
				E->address.MAC[4],
				E->address.MAC[5],
				E->IFD->name);

		InsertBTEntry (Table, E);
		PrintBT (Table);
	}
	else if (E->IFD != IFD)
		/* Adresa je znama, ale naucena na inom rozhrani - aktualizujeme. */
		E->IFD = (struct IntDescriptor *) IFD;

	E->lastSeen = time (NULL);

	return E;
}
