#ifndef PROCESS_SPAWN_H
#define PROCESS_SPAWN_H

#include "structs.h"
#include "shm.h"

typedef struct s_proc_ctx {
    t_shm    *shm;
    t_config *cfg;
    int       floor;   
    int       proc_index;  
} t_proc_ctx;

pid_t spawn_word_carrier(t_proc_ctx *ctx, int global_index);
pid_t spawn_letter_carrier(t_proc_ctx *ctx, int global_index);
pid_t spawn_sorting_proc(t_proc_ctx *ctx, int global_index);
pid_t spawn_delivery_elevator(t_shm *shm, t_config *cfg);
pid_t spawn_reposition_elevator(t_shm *shm, t_config *cfg);

int   total_process_count(t_config *cfg);
void  spawn_all(t_shm *shm, t_config *cfg,
                pid_t *pid_list, int *pid_count);

void  run_word_carrier(t_proc_ctx *ctx);
void  run_letter_carrier(t_proc_ctx *ctx);
void  run_sorting_proc(t_proc_ctx *ctx);
void  run_delivery_elev(t_shm *shm, t_config *cfg);
void  run_reposition_elev(t_shm *shm, t_config *cfg);

void  monitor_loop(t_shm *shm, t_config *cfg,
                   pid_t *pid_list, int pid_count);

#endif