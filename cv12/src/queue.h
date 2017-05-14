#ifndef QUEUE_H
#define QUEUE_H

// ========================
//      DATATYPE DECLARATIONS
// ========================

struct QueueItem {
    struct QueueItem * Next;
    void * Data;
};

struct Queue {
    struct QueueItem * head;
};

// ========================
//      FUNCTIONS
// ========================

struct Queue * initQueue(void);
struct QueueItem * enqueue(struct Queue * queue, void * data);
void  * dequeue(struct Queue * queue);
void deinitQueue(struct Queue * queue);

#endif
