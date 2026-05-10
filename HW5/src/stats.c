#include "../inc/stats.h"

#include <stdio.h>
#include <stdatomic.h>

void global_stats_init(GlobalStats *gs) {
    atomic_init(&gs->completed_orders,   0);
    atomic_init(&gs->cancelled_orders,   0);
    atomic_init(&gs->total_delivery_time, 0L);
}

void courier_stats_init(CourierStats *cs, int courier_id){
    cs->courier_id    = courier_id;
    cs->completed     = 0;
    cs->total_time_ms = 0;
}

int stats_write_file(const char            *path,
                     size_t                 total_orders,
                     const GlobalStats     *gs,
                     const CourierStats    *couriers,
                     int                    num_couriers)
{
    FILE *fp = fopen(path, "w");
    if (fp == NULL) return -1;

    int  completed = atomic_load(&gs->completed_orders);
    int  cancelled = atomic_load(&gs->cancelled_orders);
    long total_ms  = atomic_load(&gs->total_delivery_time);

    long avg_ms = (completed > 0) ? (total_ms / completed) : 0;

    fprintf(fp, "SHIFT_SUMMARY\n");
    fprintf(fp, "Total orders  : %zu\n", total_orders);
    fprintf(fp, "Completed     : %d\n",  completed);
    fprintf(fp, "Cancelled     : %d\n",  cancelled);
    fprintf(fp, "Total time    : %ldms\n", total_ms);
    fprintf(fp, "Avg per order : %ldms\n", avg_ms);
    fprintf(fp, "\n");
    fprintf(fp, "COURIER_STATS\n");
    for (int i = 0; i < num_couriers; ++i) {
        fprintf(fp, "Courier-%d completed=%d total_time=%ldms\n",
                couriers[i].courier_id,
                couriers[i].completed,
                couriers[i].total_time_ms);
    }

    fclose(fp);
    return 0;
}