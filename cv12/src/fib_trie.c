#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "queue.h"

// ========================
//		MACROS
// ========================

#define ERR_INIT_FAILED	(0x01)
#define ERR_SUCCESS	(0x02)

#define TOPBIT(x)	((x) >> 31)

#define TERMINAL_ON	(1)

#define TEST_IP_SEARCH_COUNT	(100000)
#define TEST_ENTRY_COUNT	(10000)

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"

//#define PRINT_TABLE
//#define PRINT_LOOKUP

// ========================
//		DATATYPE DECLARATIONS
// ========================

struct TrieNode {
	struct TrieNode * N[2]; // neighbors
	int Terminal;
	unsigned int Net;
	unsigned int Mask;
};

// ========================
// 		TRIE
// ========================

struct TrieNode * createEmptyNode(void){
	struct TrieNode * node = (struct TrieNode *) malloc(sizeof(struct TrieNode));
	if (!node){
		fprintf(stderr, "Error: Cannot allocate memory for Node\n");
		return NULL;
	}
	memset(node, 0, sizeof(struct TrieNode));
	return node;
}

struct TrieNode * findExisting(struct TrieNode * tab, unsigned int address, unsigned int mask){
	struct TrieNode * I = NULL;

	if (!tab){
		fprintf(stderr, "Error: NULL pointer to table in findExisting().\n");
		return NULL;
	}

	I = tab;
	while(mask) {
		I = I->N[TOPBIT(address)];
		if (!I){
			return NULL;
		}

		address <<= 1;
		mask <<= 1;
	}

	return (I->Terminal == TERMINAL_ON) ? I : NULL;
}

struct TrieNode * lookup(struct TrieNode * tab, unsigned int address){
	struct TrieNode * I = NULL;
	struct TrieNode * Match = NULL; // last stored match based on longest-prefix-match

	if (!tab){
		fprintf(stderr, "Error: NULL pointer to table in lookup().\n");
		return NULL;
	}

	I = tab; // first element in table == root
	while (I){
		if (I->Terminal == TERMINAL_ON){
			Match = I;
		}

		I = I->N[TOPBIT(address)];
		address <<= 1;
	}

	return Match;
}

struct TrieNode * addEntry(struct TrieNode * tab, unsigned int address, unsigned int mask){
	struct TrieNode * I, * node;
	unsigned int a = address, m = mask;

	if (!tab){
		fprintf(stderr, "Error: NULL pointer to table in addEntry().\n");
		return NULL;
	}

	I = tab;
	while(m){
		if (! (I->N[TOPBIT(a)])){
			node = createEmptyNode();
			if (!node){
				return NULL;
			}
			I->N[TOPBIT(a)] = node;
		}

		I = I->N[TOPBIT(a)];
		a <<= 1;
		m <<= 1;
	}

	I->Terminal = TERMINAL_ON;
	I->Net = address;
	I->Mask = mask;
	return I;
}

void printNodeInfo(struct TrieNode * node){
	if (!node){
		return;
	}

	printf("%hhu.%hhu.%hhu.%hhu/%hhu.%hhu.%hhu.%hhu\n",
			(node->Net >> 24) & 0xFF, (node->Net >> 16) & 0xFF, (node->Net >> 8) & 0xFF, (node->Net) & 0xFF,
			(node->Mask >> 24) & 0xFF, (node->Mask >> 16) & 0xFF, (node->Mask >> 8) & 0xFF, (node->Mask) & 0xFF);
}

void printTrie(struct TrieNode * tab){ // vypis tab po urovniach
	struct Queue * queue = NULL;
	struct TrieNode * node;

	if (!tab){
		fprintf(stderr, "Error: NULL table in printTrie().\n");
		return;
	}

	queue = initQueue();
	if (!queue)
		return;

	enqueue(queue, tab);

	while ((node = (struct TrieNode *) dequeue(queue))){ // kym nie je front prazdny
		if (node->Terminal == TERMINAL_ON)
			printNodeInfo(node);

		if (node->N[0])
			enqueue(queue, node->N[0]);

		if (node->N[1])
			enqueue(queue, node->N[1]);
	}

	deinitQueue(queue);
}

long int countNetworks(struct TrieNode * tab){
	struct Queue * queue = NULL;
	struct TrieNode * node;
	long int count = 0;

	if (!tab){
		fprintf(stderr, "Error: NULL table in countNetworks().\n");
		return 0;
	}

	queue = initQueue();
	if (!queue)
		return 0;

	enqueue(queue, tab);

	while ((node = (struct TrieNode *) dequeue(queue))){
		if (node->Terminal == TERMINAL_ON)
			count++;

		if (node->N[0])
			enqueue(queue, node->N[0]);

		if (node->N[1])
			enqueue(queue, node->N[1]);
	}

	deinitQueue(queue);
	return count;
}

// ========================
// 		TEST FUNCTIONS
// ========================

void genEntries(struct TrieNode * tab, int count){
	unsigned int mask, address;

	if (!tab)
		return;

	srandom(time(NULL));

	// add random entries into table
	for (int i = 0; i < count; i++){
        int maskLen = ((float) random() / RAND_MAX) * 32; // random() % 33
        unsigned int mask = 0xFFFFFFFF << maskLen;
        unsigned int address = random() & mask;
        addEntry (tab, address, mask);
	}
}

int test(void){
	struct TrieNode * tab = createEmptyNode();
	struct TrieNode * route = NULL;

	unsigned int tableSize;
	struct timeval start, end, diff;

	int ips[TEST_IP_SEARCH_COUNT];

	genEntries(tab, TEST_ENTRY_COUNT);
	tableSize = countNetworks(tab);

#ifdef PRINT_TABLE
	printTrie(tab);
	printf("\nTable size: %u\n", tableSize);
#endif

#ifdef PRINT_LOOKUP
	// search for random IPs in table
	printf("\n+-------------+\n");
	printf("|    LOOKUP   |\n");
	printf("+-------------+\n\n");
#endif

	for (int i = 0; i < TEST_IP_SEARCH_COUNT; i++){
		ips[i] = rand();
	}

	gettimeofday(&start, NULL);

	for (int i = 0; i < TEST_IP_SEARCH_COUNT; i++){
		route = lookup(tab, ips[i]);

#ifdef PRINT_LOOKUP
		if (route)
			printf(KGRN);
		else
			printf(KRED);

		printf("%hhu.%hhu.%hhu.%hhu -> ",
				(ips[i] >> 24) & 0xFF,
				(ips[i] >> 16) & 0xFF,
				(ips[i] >> 8) & 0xFF,
				(ips[i]) & 0xFF);

		if (route){
			printNodeInfo(route);
		} else {
			printf("NULL\n");
		}

		printf(KNRM);
#endif
	}

	gettimeofday(&end, NULL);
	timersub(&end, &start, &diff);

	printf("\nTotal time: %fs\tTimePerIP: %fms\tIPcount: %d\tTableSize:%ld\n", (diff.tv_sec*1000000 + diff.tv_usec) / 1000000.0, ((diff.tv_sec*1000000 + diff.tv_usec) / (float) TEST_IP_SEARCH_COUNT) / 1000.0, TEST_IP_SEARCH_COUNT, countNetworks(tab));

	return ERR_SUCCESS;
}

// =======================
//		MAIN function definition
// =======================

int main(int argc, char ** argv){
	return test();
}
