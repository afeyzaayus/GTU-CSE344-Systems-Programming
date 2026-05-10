#ifndef STATS_H
#define STATS_H

#include <stdatomic.h>
#include <stddef.h>

typedef struct {
    _Atomic int  completed_orders;     
    _Atomic int  cancelled_orders;    
    _Atomic long total_delivery_time; 
} GlobalStats;

void global_stats_init(GlobalStats *gs);

typedef struct {
    int  courier_id; 
    int  completed;
    long total_time_ms;
} CourierStats;

void courier_stats_init(CourierStats *cs, int courier_id);

int stats_write_file(const char            *path,
                     size_t                 total_orders,
                     const GlobalStats     *gs,
                     const CourierStats    *couriers,
                     int                    num_couriers);

#endif 