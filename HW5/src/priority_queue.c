#include "../inc/priority_queue.h"
#include "../inc/order.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

struct PriorityQueue {
    Order  *heap;       
    size_t  size;        
    size_t  capacity;    
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    bool shutdown;       
};

static bool order_less(const Order *a, const Order *b){
    if (a->priority != b->priority) return a->priority < b->priority;
    return a->id < b->id;
}

static void swap_at(Order *heap, size_t i, size_t j){
    Order tmp = heap[i];
    heap[i]   = heap[j];
    heap[j]   = tmp;
}

static void sift_up(Order *heap, size_t i){
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (order_less(&heap[i], &heap[parent])) {
            swap_at(heap, i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

static void sift_down(Order *heap, size_t size, size_t i){
    for (;;) {
        size_t left  = 2 * i + 1;
        size_t right = 2 * i + 2;
        size_t best  = i;

        if (left  < size && order_less(&heap[left],  &heap[best])) best = left;
        if (right < size && order_less(&heap[right], &heap[best])) best = right;

        if (best == i) break;   
        swap_at(heap, i, best);
        i = best;
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

PriorityQueue *pq_create(size_t capacity)
{
    if (capacity == 0) return NULL;

    PriorityQueue *pq = malloc(sizeof(*pq));
    if (pq == NULL) return NULL;

    pq->heap = malloc(sizeof(Order) * capacity);
    if (pq->heap == NULL) {
        free(pq);
        return NULL;
    }

    pq->size     = 0;
    pq->capacity = capacity;
    pq->shutdown = false;

    /* pthread_mutex_init / cond_init can technically fail (e.g. ENOMEM
     * on resource-starved systems). Treat that as fatal-for-this-object. */
    if (pthread_mutex_init(&pq->mutex, NULL) != 0) {
        free(pq->heap);
        free(pq);
        return NULL;
    }
    if (pthread_cond_init(&pq->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&pq->mutex);
        free(pq->heap);
        free(pq);
        return NULL;
    }

    return pq;
}

void pq_destroy(PriorityQueue *pq)
{
    if (pq == NULL) return;
    pthread_cond_destroy(&pq->not_empty);
    pthread_mutex_destroy(&pq->mutex);
    free(pq->heap);
    free(pq);
}

int pq_push(PriorityQueue *pq, const Order *order)
{
    if (pq == NULL || order == NULL) return -1;

    pthread_mutex_lock(&pq->mutex);

    if (pq->size >= pq->capacity) {
        pthread_mutex_unlock(&pq->mutex);
        return -1;  /* caller bug: capacity was sized wrong */
    }

    /* Append at the end, then sift-up to restore heap property. */
    pq->heap[pq->size] = *order;
    sift_up(pq->heap, pq->size);
    pq->size++;

    /* Wake one waiting consumer (if any). cond_signal is a no-op when
     * nobody is waiting, so it's always safe to call. */
    pthread_cond_signal(&pq->not_empty);

    pthread_mutex_unlock(&pq->mutex);
    return 0;
}

pq_pop_result pq_pop_blocking(PriorityQueue *pq, Order *out)
{
    if (pq == NULL || out == NULL) return PQ_POP_SHUTDOWN;

    pthread_mutex_lock(&pq->mutex);

    /* Wait while: queue is empty AND shutdown not requested.
     * Loop guards against spurious wakeups (per pthread spec). */
    while (pq->size == 0 && !pq->shutdown) {
        pthread_cond_wait(&pq->not_empty, &pq->mutex);
    }

    /* Two ways to exit the wait:
     *   1. Queue has an item -> pop it, return OK.
     *   2. Queue empty AND shutdown -> return SHUTDOWN.
     *
     * If shutdown was requested but items remain, we still process them
     * here; main is responsible for draining via pq_drain() if it wants
     * to cancel pending orders instead. (The HW flow uses pq_drain()
     * during shutdown, so couriers never actually pop after shutdown.) */
    if (pq->size == 0) {
        /* shutdown && empty */
        pthread_mutex_unlock(&pq->mutex);
        return PQ_POP_SHUTDOWN;
    }

    /* Take root (highest-priority item), move last item to root, sift down. */
    *out = pq->heap[0];
    pq->size--;
    if (pq->size > 0) {
        pq->heap[0] = pq->heap[pq->size];
        sift_down(pq->heap, pq->size, 0);
    }

    pthread_mutex_unlock(&pq->mutex);
    return PQ_POP_OK;
}

void pq_shutdown(PriorityQueue *pq)
{
    if (pq == NULL) return;

    pthread_mutex_lock(&pq->mutex);
    pq->shutdown = true;
    /* Wake EVERY waiting consumer — they'll re-check the predicate and
     * either pop a remaining item (rare; main usually drains first) or
     * see shutdown==true with size==0 and return SHUTDOWN. */
    pthread_cond_broadcast(&pq->not_empty);
    pthread_mutex_unlock(&pq->mutex);
}

size_t pq_size(PriorityQueue *pq)
{
    if (pq == NULL) return 0;
    pthread_mutex_lock(&pq->mutex);
    size_t s = pq->size;
    pthread_mutex_unlock(&pq->mutex);
    return s;
}

void pq_drain(PriorityQueue *pq, Order *out, size_t *out_count)
{
    if (pq == NULL || out_count == NULL) {
        if (out_count) *out_count = 0;
        return;
    }

    pthread_mutex_lock(&pq->mutex);

    /* Pop one at a time in priority order. We could just memcpy the
     * underlying array, but that wouldn't preserve priority order
     * (heap layout != sorted layout). Drain via repeated extract-min
     * which IS sorted output. N is small, O(N log N) is fine. */
    size_t n = 0;
    while (pq->size > 0) {
        out[n++] = pq->heap[0];
        pq->size--;
        if (pq->size > 0) {
            pq->heap[0] = pq->heap[pq->size];
            sift_down(pq->heap, pq->size, 0);
        }
    }

    *out_count = n;
    pthread_mutex_unlock(&pq->mutex);
}