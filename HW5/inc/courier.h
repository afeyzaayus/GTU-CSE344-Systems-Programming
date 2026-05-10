#ifndef COURIER_H
#define COURIER_H

#include "priority_queue.h"
#include "stats.h"

typedef struct {
    int            courier_id;    
    PriorityQueue *pq;    
    GlobalStats   *gs;       
    CourierStats   stats;        
} CourierContext;

void *courier_thread_main(void *arg);

#endif 