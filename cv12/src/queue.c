#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

struct Queue * initQueue(void){ // vytvor prazdny front
    struct Queue * q = (struct Queue *) malloc(sizeof(struct Queue));
    if (!q){
        return NULL;
    }

    q->head = NULL;
    return q;
}

struct QueueItem * enqueue(struct Queue * queue, void * data){ // vloz novy prvok na zaciatok frontu
    struct QueueItem * I;

    if (!queue){
        fprintf(stderr, "Error: NULL table in enqueue().\n");
        return NULL;
    }

    I = (struct QueueItem *) malloc(sizeof(struct QueueItem));
    if (!I){
        return NULL;
    }

    I->Data = data;

    if (!(queue->head)){
        I->Next = NULL;
        queue->head = I;
    } else {
        I->Next = queue->head;
        queue->head = I;
    }

    return I;
}

void  * dequeue(struct Queue * queue){
    struct QueueItem * I, * prev;
    void * data;

    if (!queue){
        fprintf(stderr, "Error: NULL table in dequeue().\n");
        return NULL; // neexistujuci front
    }

    if (!(queue->head)){ // front je prazdny
        return NULL;
    }

    prev = NULL;
    I = queue->head; // prvy prvok

    while(I->Next){
        prev = I;
        I = I->Next;
    }

    if (!prev){ // front obsahuje len jeden prvok
        queue->head = NULL;
    } else {
        prev->Next = NULL; // odrez posledny prvok z frontu
    }

    data = I->Data;
    free(I);

    return data;
}

void deinitQueue(struct Queue * queue){
    struct QueueItem *I, *tmp;

    if (!queue) {
        fprintf(stderr, "Error: NULL table in deinitQueue().\n");
        return;
    }

    I = queue->head;
    while(I){
        tmp = I;
        I = I->Next;
        free(tmp);
    }
}
