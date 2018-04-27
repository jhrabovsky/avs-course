#include "routing-table-trie.h"

struct Table * createTable(void){
	struct Table * table = (struct Table *) malloc(sizeof(struct Table));

	if (table == NULL){
		fprintf(stderr, "createTable() : Table could not be created.\n");
	} else {
		table->count = 0;
		table->root = NULL;
	}

	return table;
}

struct Node * lookupRoute(struct Table * table, uint32_t address){
	struct Node * node;
	struct Node * lastFound = NULL;

	if (table == NULL){
		fprintf(stderr, "lookupRoute() : Table does not exist.\n");
		return NULL;
	}

	node = table->root;

	while(node != NULL){
		if (node->terminal == TERMINAL_ON){
			lastFound = node;
		}

		node = node->next[TOP_BIT(address)];
		address = address << 1;
	}

	return lastFound;
}

struct Node * findRoute(struct Table * table, uint32_t net, uint32_t mask){
	struct Node * node;

	if (table == NULL){
		fprintf(stderr, "findRoute() : Table does not exist.\n");
		return NULL;
	}

	node = table->root;

	while(mask != 0){
		if (node == NULL){
			break;
		}

		node = node->next[TOP_BIT(net)];
		mask = mask << 1;
		net = net << 1;
	}

	return node;
}

struct Node * addRoute(struct Table * table, uint32_t net, uint32_t mask){
	struct Node * node;
	uint32_t net_tmp = net;
	uint32_t mask_tmp = mask;


	if (table == NULL){
		fprintf(stderr, "addRoute() : Table does not exist.\n");
		return NULL;
	}

	if (table->root == NULL){
		table->root = (struct Node *) malloc(sizeof(struct Node));
		if (table->root == NULL){
			perror("addRoute()");
			exit(EXIT_FAILURE);
		}
		memset(table->root, 0, sizeof(struct Node));
	}

	node = table->root;

	while(mask_tmp != 0){
		if (node->next[TOP_BIT(net_tmp)] == NULL){
			node->next[TOP_BIT(net_tmp)] = (struct Node *) malloc(sizeof(struct Node));
			if (node->next[TOP_BIT(net_tmp)] == NULL){
				perror("addRoute()");
				exit(EXIT_FAILURE);
			}
			memset(node->next[TOP_BIT(net_tmp)], 0, sizeof(struct Node));
		}

		node = node->next[TOP_BIT(net_tmp)];
		mask_tmp = mask_tmp << 1;
		net_tmp = net_tmp << 1;
	}

	node->net = net;
	node->mask = mask;
	if (node->terminal != TERMINAL_ON){
		node->terminal = TERMINAL_ON;
		table->count++;
	}

	return node;
}

struct Table * flushTable(struct Table * table){
	struct Node * node;

	if (table == NULL){
		fprintf(stderr, "flushTable() : Table does not exist.\n");
		return table;
	}

	if(table->root == NULL){
		table->count = 0;
		return table;
	}

	struct Queue * queue = initQueue();

	if (queue == NULL){
		fprintf(stderr, "printTable() : Queue cannot be created.\n");
		return table;
	}

	enqueue(queue, (void *) table->root);

	while((node = (struct Node *) dequeue(queue)) != NULL){
		if (node->next[0] != NULL){
			enqueue(queue, (void *) node->next[0]);
		}

		if (node->next[1] != NULL){
			enqueue(queue, (void *) node->next[1]);
		}

		memset(node, 0, sizeof(struct Node));
		free(node);
	}

	deinitQueue(queue);

	return table;
}

void printNode(struct Node * node){
	if (node != NULL){
		printf("%hhu.%hhu.%hhu.%hhu/%hhu.%hhu.%hhu.%hhu\n",
			(node->net >> 24) & 0xFF,
			(node->net >> 16) & 0xFF,
			(node->net >> 8) & 0xFF,
			(node->net) & 0xFF,
			(node->mask >> 24) & 0xFF,
			(node->mask >> 16) & 0xFF,
			(node->mask >> 8) & 0xFF,
			(node->mask) & 0xFF);
	}
}

void printTable(struct Table * table){
	struct Node * node;

	if (table == NULL){
		fprintf(stderr, "printTable() : Table does not exist.\n");
		return;
	}

	if(table->root == NULL){
		return;
	}

	struct Queue * queue = initQueue();

	if (queue == NULL){
		fprintf(stderr, "printTable() : Queue cannot be created.\n");
		return;
	}

	enqueue(queue, (void *) table->root);

	while((node = (struct Node *) dequeue(queue)) != NULL){
		if (node->terminal == TERMINAL_ON){
			printNode(node);
		}

		if (node->next[0] != NULL){
			enqueue(queue, (void *) node->next[0]);
		}

		if (node->next[1] != NULL){
			enqueue(queue, (void *) node->next[1]);
		}
	}

	deinitQueue(queue);
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
	struct Node * node;
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
    	node = lookupRoute(table, addresses[i]);

#ifdef PRINT_LOOKUP
    	printf ("%hhu.%hhu.%hhu.%hhu -> ",
    	    (addresses[i] >> 24) & 0xFF,
    	    (addresses[i] >> 16) & 0xFF,
    	    (addresses[i] >> 8) & 0xFF,
    	    (addresses[i]) & 0xFF);
#endif

    	if (node == NULL){
#ifdef PRINT_LOOKUP
    		printf("none\n");
#endif
    	} else {

    		matchCount++;

#ifdef PRINT_LOOKUP
    		printNode(node);
#endif
    	}
    }

    gettimeofday(&stop, NULL); // vypnem stopky
    timersub(&stop, &start, &duration); // odmeriam cas trvania
    printf("\nDONE.\nTIME: %ld sec %ld usec, hits: %d, ratio: %f, avg lookup: %f usec/pkt, table size: %d\n",
        duration.tv_sec, duration.tv_usec, matchCount,
        ((float) matchCount) / ADDRESS_COUNT * 100,
        (float) (duration.tv_sec * 1000000 + duration.tv_usec) / ADDRESS_COUNT,
        table->count);

    // vycistenie a odstranenie tabulky
	flushTable(table);
	free(table);

	return EXIT_SUCCESS;
}
