#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "order.h"
#include <stddef.h>

typedef struct PriorityQueue PriorityQueue;

typedef enum {
    PQ_POP_OK       = 0,  
    PQ_POP_SHUTDOWN = 1  
} pq_pop_result;

PriorityQueue *pq_create(size_t capacity);
void pq_destroy(PriorityQueue *pq);
int pq_push(PriorityQueue *pq, const Order *order);
pq_pop_result pq_pop_blocking(PriorityQueue *pq, Order *out);
void pq_shutdown(PriorityQueue *pq);
size_t pq_size(PriorityQueue *pq);
void pq_drain(PriorityQueue *pq, Order *out, size_t *out_count);

#endif 