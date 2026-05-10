#include "../inc/courier.h"
#include "../inc/priority_queue.h"
#include "../inc/stats.h"
#include "../inc/log.h"
#include "../inc/order.h"

#include <time.h>
#include <errno.h>
#include <stdatomic.h>

static void sleep_ms_uninterruptible(long ms){
    if (ms <= 0) return;

    struct timespec req;
    req.tv_sec  = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;  

    struct timespec rem;
    while (nanosleep(&req, &rem) == -1) {
        if (errno != EINTR) {
            break;
        }
        req = rem; 
    }
}

void *courier_thread_main(void *arg){
    CourierContext *ctx = (CourierContext *)arg;

    for (;;) {
        log_printf("[COURIER-%d] WAITING", ctx->courier_id);

        Order order;
        pq_pop_result r = pq_pop_blocking(ctx->pq, &order);
        if (r == PQ_POP_SHUTDOWN) {
            break;
        }

        log_printf("[COURIER-%d] DELIVERY_START id=%d recipient=%s priority=%s",
                   ctx->courier_id,
                   order.id,
                   order.name,
                   priority_to_string(order.priority));

        long duration_ms = (long)order.duration * SIM_UNIT_MS;
        sleep_ms_uninterruptible(duration_ms);

        log_printf("[COURIER-%d] DELIVERY_COMPLETE id=%d recipient=%s duration=%ldms",
                   ctx->courier_id,
                   order.id,
                   order.name,
                   duration_ms);

        atomic_fetch_add(&ctx->gs->completed_orders,    1);
        atomic_fetch_add(&ctx->gs->total_delivery_time, duration_ms);

        ctx->stats.completed++;
        ctx->stats.total_time_ms += duration_ms;
    }

    log_printf("[COURIER-%d] SHIFT_OVER", ctx->courier_id);
    return NULL;
}