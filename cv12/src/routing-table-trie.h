#ifndef ROUTING_TABLE_TRIE
#define ROUTING_TABLE_TRIE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "queue.h"

#define TOP_BIT(x)  ((x) >> 31)
#define TERMINAL_ON  (1)

#define MAX_ROUTES (50) // pocet generovanych tras v tabulke => kvoli moznosti opakovaneho generovania tej istej hodnoty je toto cislo len horna hranica, skutocnych tras v tabulke bude menej
#define ADDRESS_COUNT (50) // pocet generovanych IP adries pre testovanie vyhladavania v tabulke
#define GENERATE_ROUTES_SEED (123456) // nastavenie generatora pre pridavanie nahodnych tras do tabulky

#define PRINT_TABLE
#define PRINT_LOOKUP

struct Node {
	struct Node * next[2];
	uint32_t net;
	uint32_t mask;
	uint8_t terminal;
};

struct Table {
	struct Node * root;
	unsigned int count;
};

struct Table * createTable(void);

struct Node * findRoute(struct Table * table, uint32_t net, uint32_t mask);

struct Node * addRoute(struct Table * table, uint32_t net, uint32_t mask);

struct Table * flushTable(struct Table * table);

struct Node * lookupRoute(struct Table * table, uint32_t address);

void printNode(struct Node * node);

void printTable(struct Table * table);

#endif
