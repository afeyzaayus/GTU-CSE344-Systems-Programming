#include "./inc/parser.h"
#include "./inc/order.h"
#include "./inc/priority_queue.h"
#include "./inc/courier.h"
#include "./inc/stats.h"
#include "./inc/log.h"
#include "./inc/signal_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

static void enqueue_all_orders(PriorityQueue *pq, const Order *orders, size_t n_orders){
    for (size_t i = 0; i < n_orders; ++i) {
        log_printf("[CARGOGTU] ORDER_QUEUED id=%d recipient=%s priority=%s duration=%d",
                   orders[i].id,
                   orders[i].name,
                   priority_to_string(orders[i].priority),
                   orders[i].duration);
        if (pq_push(pq, &orders[i]) != 0) {
            fprintf(stderr, "internal error: pq_push failed\n");
        }
    }
}

static int spawn_couriers(pthread_t *threads,
                          CourierContext *ctxs,
                          int num_couriers,
                          PriorityQueue *pq,
                          GlobalStats *gs)
{
    for (int i = 0; i < num_couriers; ++i) {
        ctxs[i].courier_id = i + 1; 
        ctxs[i].pq         = pq;
        ctxs[i].gs         = gs;
        courier_stats_init(&ctxs[i].stats, i + 1);

        if (pthread_create(&threads[i], NULL, courier_thread_main, &ctxs[i]) != 0) {
            fprintf(stderr, "error: pthread_create failed for courier %d\n", i + 1);
            pq_shutdown(pq);
            for (int j = 0; j < i; ++j) pthread_join(threads[j], NULL);
            return -1;
        }
    }
    return 0;
}

static void wait_for_shift_end(const GlobalStats *gs, size_t n_orders){
    while (!g_shutdown_requested &&
           atomic_load(&gs->completed_orders) < (int)n_orders) {
        struct timespec ts = {0, 50 * 1000 * 1000};  /* 50 ms */
        nanosleep(&ts, NULL);
    }
}

static void handle_shutdown_cancellations(PriorityQueue *pq,
                                          GlobalStats *gs,
                                          size_t pq_cap)
{
    if (!g_shutdown_requested) return;

    Order *pending = calloc(pq_cap, sizeof(Order));
    if (pending == NULL) {
        log_printf("[CARGOGTU] SIGINT_RECEIVED pending_orders=0");
        return;
    }

    size_t n_pending = 0;
    pq_drain(pq, pending, &n_pending);

    log_printf("[CARGOGTU] SIGINT_RECEIVED pending_orders=%zu", n_pending);

    for (size_t i = 0; i < n_pending; ++i) {
        log_printf("[CARGOGTU] ORDER_CANCELLED id=%d recipient=%s priority=%s",
                   pending[i].id,
                   pending[i].name,
                   priority_to_string(pending[i].priority));
        atomic_fetch_add(&gs->cancelled_orders, 1);
    }

    free(pending);
}

static void write_final_stats(const char *stats_path,
                              size_t n_orders,
                              const GlobalStats *gs,
                              const CourierContext *ctxs,
                              int num_couriers)
{
    CourierStats *flat = calloc((size_t)num_couriers, sizeof(CourierStats));
    if (flat == NULL) {
        fprintf(stderr, "warning: cannot allocate stats buffer\n");
        return;
    }

    for (int i = 0; i < num_couriers; ++i) flat[i] = ctxs[i].stats;

    if (stats_write_file(stats_path, n_orders, gs, flat, num_couriers) != 0) {
        fprintf(stderr, "warning: cannot write stats file '%s'\n", stats_path);
    }

    free(flat);
}

int main(int argc, char **argv){
    int   num_couriers;
    char *input_path;
    char *stats_path;
    if (parse_cli_args(argc, argv, &num_couriers, &input_path, &stats_path) != 0) {
        return EXIT_FAILURE;
    }

    Order *orders   = NULL;
    size_t n_orders = 0;
    if (parse_orders_file(input_path, &orders, &n_orders) != 0) {
        fprintf(stderr, "error: cannot open input file '%s'\n", input_path);
        return EXIT_FAILURE;
    }

    log_init();

    if (install_sigint_handler() != 0) {
        fprintf(stderr, "error: cannot install SIGINT handler\n");
        free(orders);
        log_destroy();
        return EXIT_FAILURE;
    }

    GlobalStats gs;
    global_stats_init(&gs);

    size_t pq_cap = (n_orders > 0) ? n_orders : 1;
    PriorityQueue *pq = pq_create(pq_cap);
    if (pq == NULL) {
        fprintf(stderr, "error: cannot create priority queue\n");
        free(orders);
        log_destroy();
        return EXIT_FAILURE;
    }

    log_printf("[CARGOGTU] SHIFT_START couriers=%d orders=%zu",
               num_couriers, n_orders);
    enqueue_all_orders(pq, orders, n_orders);

    pthread_t      *threads = calloc((size_t)num_couriers, sizeof(pthread_t));
    CourierContext *ctxs    = calloc((size_t)num_couriers, sizeof(CourierContext));
    if (threads == NULL || ctxs == NULL) {
        fprintf(stderr, "error: cannot allocate thread state\n");
        free(threads); free(ctxs);
        pq_destroy(pq); free(orders); log_destroy();
        return EXIT_FAILURE;
    }

    if (spawn_couriers(threads, ctxs, num_couriers, pq, &gs) != 0) {
        free(threads); free(ctxs);
        pq_destroy(pq); free(orders); log_destroy();
        return EXIT_FAILURE;
    }

    wait_for_shift_end(&gs, n_orders);
    handle_shutdown_cancellations(pq, &gs, pq_cap);

    pq_shutdown(pq);
    for (int i = 0; i < num_couriers; ++i) {
        pthread_join(threads[i], NULL);
    }

    int  completed = atomic_load(&gs.completed_orders);
    int  cancelled = atomic_load(&gs.cancelled_orders);
    long total_ms  = atomic_load(&gs.total_delivery_time);

    log_printf("[CARGOGTU] SHIFT_END completed=%d cancelled=%d total_time=%ldms",
               completed, cancelled, total_ms);

    write_final_stats(stats_path, n_orders, &gs, ctxs, num_couriers);

    log_printf("[CARGOGTU] SHUTDOWN_COMPLETE");

    free(threads);
    free(ctxs);
    pq_destroy(pq);
    free(orders);
    log_destroy();
    return EXIT_SUCCESS;
}