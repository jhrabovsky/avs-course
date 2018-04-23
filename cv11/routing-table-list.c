#include "routing-table-list.h"

struct Table * createTable(void){
	struct Table * table = (struct Table *) malloc(sizeof(struct Table));

	if (table == NULL){		
		fprintf(stderr, "createTable() : Table could not be created.\n");
	} else {
		table->count = 0;
		table->head = NULL;
	}

	return table;
}

struct Route * lookupRoute(struct Table * table, uint32_t address){
	struct Route * route;
	uint32_t net;

	if (table == NULL){
		fprintf(stderr, "lookupRoute() : Table does not exist.\n");
		return NULL;
	}

	route = table->head;

	while(route != NULL){
		net = address & route->mask;
		if (net == route->net){
			break;
		}
		route = route->next;
	}

	return route;
}

struct Route * findRoute(struct Table * table, uint32_t net, uint32_t mask){
	struct Route * route;

	if (table == NULL){
		fprintf(stderr, "findRoute() : Table does not exist.\n");
		return NULL;
	}

	route = table->head;

	while(route != NULL){
		if ((net == route->net) && (mask == route->mask)){
			break;
		}
		route = route->next;
	}

	return route;
}

// trasy vkladam do tabulky v poradi od najdlhsieho prefixu po najkratsi => zjednodusene neskorsie vyhladavanie podla principu LONGEST-PREFIX-MATCH 
struct Route * addRoute(struct Table * table, uint32_t net, uint32_t mask){
	struct Route * route;

	if (table == NULL){
		fprintf(stderr, "addRoute() : Table does not exist.\n");
		return NULL;
	}

	route = findRoute(table, net, mask);
	if (route != NULL){
		fprintf(stderr, "addRoute() : Route already exists.\n");
		return route;
	}

	route = (struct Route *) malloc(sizeof(struct Route));
	if (route == NULL){
		fprintf(stderr, "addRoute() : Memory unavailable.\n");
		return NULL;
	}

	route->next = NULL;
	route->net = net;
	route->mask = mask;

	struct Route * item = table->head;

	if (item == NULL){ // vlozenie zaznamu do prazdnej tabulky
		table->head = route;
	} else if (route->mask >= item->mask){ // vlozenie zaznamu hned na zaciatok pred aktualny table->head
		route->next = item;
		table->head = route;
	} else {
		while(item->next != NULL){
			if (route->mask >= item->next->mask){ // vlozenie zaznamu pred taku trasu, kde dlzka prefixu je rovna alebo uz mensia ako dlzka novej trasy
				break;
			}
			item = item->next;
		}

		route->next = item->next;
		item->next = route;
	}

	table->count++;
	return route;
}

struct Table * flushTable(struct Table * table){
	struct Route * route, * item;

	if (table == NULL){
		fprintf(stderr, "flushTable() : Table does not exist.\n");
		return NULL;
	}

	route = table->head;
	while(route != NULL){
		item = route->next;
		route->mask = 0;
		route->net = 0;
		route->next = NULL;
		free(route);

		route = item;
	}

	table->count = 0;
	table->head = NULL;
	return table;
}

void printTable(struct Table * table){
	struct Route * route;

	if (table == NULL){
		fprintf(stderr, "printTable() : Table does not exist.\n");
		return;
	}

	printf("+-------------------+\n");
	printf("|   Routing Table   |\n");
	printf("+-------------------+\n\n");

	route = table->head;
	
	while(route != NULL){
		printf("%hhu.%hhu.%hhu.%hhu/%hhu.%hhu.%hhu.%hhu\n",
		            (route->net >> 24) & 0xFF,
		            (route->net >> 16) & 0xFF,
		            (route->net >> 8) & 0xFF,
		            (route->net) & 0xFF,
		            (route->mask >> 24) & 0xFF,
		            (route->mask >> 16) & 0xFF,
		            (route->mask >> 8) & 0xFF,
		            (route->mask) & 0xFF);

		route = route->next;
	}
}

// naplnenie tabulky nahodnymi trasami, parameter "seed" sluzi kvoli opakovanemu generovaniu tych istych tras => testovanie/debugovanie
void generateNetworks(struct Table * table, unsigned int count, unsigned int seed){
	uint32_t net, mask, len;

	if (table == NULL){
		fprintf(stderr, "generateNetworks() : Table does not exist.\n");
		return;
	}

	srandom(seed);

	for (int i = 0; i < count; i++){
		len = ((float) random() / RAND_MAX) * 32;
		mask = 0xFFFFFFFF << (32-len);
		net = random() & mask;

		addRoute(table, net, mask);
	}
}

int main(void){
	struct Table * table;
	struct Route * route;
	unsigned int addresses[ADDRESS_COUNT]; // pole nahodnych IP adries pre testovanie vyhladavania v tabulke
	struct timeval start, stop, duration;
	unsigned int matchCount = 0;

	table = createTable();
	if (table == NULL){
		return EXIT_FAILURE;
	}

	generateNetworks(table, MAX_ROUTES, GENERATE_ROUTES_SEED);

#ifdef PRINT_TABLE
	printTable(table);
#endif

	srandom(time(NULL));

	for (int i = 0; i < ADDRESS_COUNT; i++){
		addresses[i] = random();
	}

#ifdef PRINT_LOOKUP
    printf("\n+-------------+\n");
    printf("|    LOOKUP   |\n");
    printf("+-------------+\n\n");
#endif

    fflush(stdout);
    
    gettimeofday(&start, NULL); // zapnem stopky

    for(int i = 0; i < ADDRESS_COUNT; i++){
    	route = lookupRoute(table, addresses[i]);

#ifdef PRINT_LOOKUP
    	printf ("%hhu.%hhu.%hhu.%hhu -> ",
    	    (addresses[i] >> 24) & 0xFF,
    	    (addresses[i] >> 16) & 0xFF,
    	    (addresses[i] >> 8) & 0xFF,
    	    (addresses[i]) & 0xFF);
#endif

    	if (route == NULL){
#ifdef PRINT_LOOKUP
    		printf("none\n");
#endif
    	} else {

    		matchCount++;

#ifdef PRINT_LOOKUP
    		printf ("%hhu.%hhu.%hhu.%hhu/%hhu.%hhu.%hhu.%hhu\n",
    		    (route->net >> 24) & 0xFF,
    		    (route->net >> 16) & 0xFF,
    		    (route->net >> 8) & 0xFF,
	            (route->net) & 0xFF,
	            (route->mask >> 24) & 0xFF,
	            (route->mask >> 16) & 0xFF,
	            (route->mask >> 8) & 0xFF,
	            (route->mask) & 0xFF);
#endif
    	}
    }

    gettimeofday(&stop, NULL); // vypnem stopky
    timersub(&stop, &start, &duration); // odmeriam cas trvania
    printf("DONE.\nTIME: %ld sec %ld usec, hits: %d, ratio: %f, avg lookup: %f usec/pkt, table size: %d\n",
        duration.tv_sec, duration.tv_usec, matchCount,
        ((float) matchCount) / ADDRESS_COUNT * 100,
        (float) (duration.tv_sec * 1000000 + duration.tv_usec) / ADDRESS_COUNT,
        table->count);

    // vycistenie a odstranenie tabulky
	flushTable(table);
	free(table);

	return EXIT_SUCCESS;
}
