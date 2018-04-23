#ifndef ROUTING_TABLE_LIST
#define ROUTING_TABLE_LIST

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define MAX_ROUTES (100) // pocet generovanych tras v tabulke => kvoli moznosti opakovaneho generovania tej istej hodnoty je toto cislo len horna hranica, skutocnych tras v tabulke bude menej
#define ADDRESS_COUNT (10) // pocet generovanych IP adries pre testovanie vyhladavania v tabulke
#define GENERATE_ROUTES_SEED (123456) // nastavenie generatora pre pridavanie nahodnych tras do tabulky

#define PRINT_TABLE
#define PRINT_LOOKUP

struct Route {
	struct Route * next;
	uint32_t net;
	uint32_t mask;
};

struct Table {
	struct Route * head;
	unsigned int count;
};

struct Table * createTable(void);

struct Route * findRoute(struct Table * table, uint32_t net, uint32_t mask);

struct Route * addRoute(struct Table * table, uint32_t net, uint32_t mask);

struct Table * flushTable(struct Table * table);

struct Route * lookupRoute(struct Table * table, uint32_t address);

void printTable(struct Table * table);

#endif
