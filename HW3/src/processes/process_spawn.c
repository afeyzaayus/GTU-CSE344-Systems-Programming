#include "../../inc/process_spawn.h"
#include "../../inc/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

int total_process_count(t_config *cfg)
{
    int per_floor = cfg->word_carriers_per_floor
                  + cfg->letter_carriers_per_floor
                  + cfg->sorting_processes_per_floor;
    return per_floor * cfg->num_floors + 2;
}

pid_t spawn_word_carrier(t_proc_ctx *ctx, int global_index)
{
    pid_t pid = fork();
    if (pid < 0) { perror("ERROR: fork word_carrier"); exit(EXIT_FAILURE); }
    if (pid == 0){
        log_fmt("[PID:%d] Word-carrier-process_%d initialized on floor %d\n",
                getpid(), global_index, ctx->floor);
        run_word_carrier(ctx);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

pid_t spawn_letter_carrier(t_proc_ctx *ctx, int global_index)
{
    pid_t pid = fork();
    if (pid < 0) { perror("ERROR: fork letter_carrier"); exit(EXIT_FAILURE); }
    if (pid == 0){
        int lc_idx = ctx->floor * ctx->cfg->letter_carriers_per_floor + ctx->proc_index;
        ctx->shm->lc_states[lc_idx].pid          = getpid();
        ctx->shm->lc_states[lc_idx].current_floor = ctx->floor;
        ctx->shm->lc_states[lc_idx].idle          = 0;

        log_fmt("[PID:%d] Letter-carrier-process_%d initialized on floor %d\n",
                getpid(), global_index, ctx->floor);
        run_letter_carrier(ctx);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

pid_t spawn_sorting_proc(t_proc_ctx *ctx, int global_index)
{
    pid_t pid = fork();
    if (pid < 0) { perror("ERROR: fork sorting_proc"); exit(EXIT_FAILURE); }
    if (pid == 0) {
        log_fmt("[PID:%d] Sorting-process_%d initialized on floor %d\n",
                getpid(), global_index, ctx->floor);
        run_sorting_proc(ctx);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

pid_t spawn_delivery_elevator(t_shm *shm, t_config *cfg)
{
    pid_t pid = fork();
    if (pid < 0) { perror("ERROR: fork delivery_elevator"); exit(EXIT_FAILURE); }
    if (pid == 0){
        log_fmt("[PID:%d] Delivery elevator process started\n", getpid());
        run_delivery_elev(shm, cfg);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

pid_t spawn_reposition_elevator(t_shm *shm, t_config *cfg)
{
    pid_t pid = fork();
    if (pid < 0) { perror("ERROR: fork reposition_elevator"); exit(EXIT_FAILURE); }
    if (pid == 0){
        log_fmt("[PID:%d] Reposition elevator process started\n", getpid());
        run_reposition_elev(shm, cfg);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

void spawn_all(t_shm *shm, t_config *cfg, pid_t *pid_list, int *pid_count)
{
    int        floor, i, idx = 0;
    int        wc_global = 0, lc_global = 0, sp_global = 0;
    t_proc_ctx ctx;

    ctx.shm = shm;
    ctx.cfg = cfg;

    log_fmt("[PID:%d] Parent process started\n", getpid());
    log_msg("Processes are being created...\n");

    for (floor = 0; floor < cfg->num_floors; floor++){
        log_fmt("--- Initializing Floor %d ---\n", floor);
        ctx.floor = floor;

        for (i = 0; i < cfg->word_carriers_per_floor; i++){
            ctx.proc_index = i;
            pid_list[idx++] = spawn_word_carrier(&ctx, wc_global++);
        }
        for (i = 0; i < cfg->letter_carriers_per_floor; i++){
            ctx.proc_index = i;
            pid_list[idx++] = spawn_letter_carrier(&ctx, lc_global++);
        }
        for (i = 0; i < cfg->sorting_processes_per_floor; i++){
            ctx.proc_index = i;
            pid_list[idx++] = spawn_sorting_proc(&ctx, sp_global++);
        }
    }

    pid_list[idx++] = spawn_delivery_elevator(shm, cfg);
    pid_list[idx++] = spawn_reposition_elevator(shm, cfg);
    *pid_count = idx;

    log_msg("--------------------------------------------------\n");
}

static int count_live_lc_on_floor(t_shm *shm, t_config *cfg, int floor)
{
    int total_lc = cfg->num_floors * cfg->letter_carriers_per_floor;
    int count    = 0;
    int i;

    for (i = 0; i < total_lc; i++){
        pid_t pid = shm->lc_states[i].pid;
        if (pid <= 0)
            continue;
        if (shm->lc_states[i].current_floor != floor)
            continue;
        int ret = waitpid(pid, NULL, WNOHANG);
        if (ret == 0)
            count++;
        else
            shm->lc_states[i].pid = 0;
    }
    return count;
}

typedef struct s_pid_dyn {
    pid_t *list;
    int    count;
    int    cap;
} t_pid_dyn;

static void dyn_push(t_pid_dyn *d, pid_t pid)
{
    if (d->count >= d->cap)
    {
        int    new_cap  = d->cap ? d->cap * 2 : 8;
        pid_t *new_list = realloc(d->list, sizeof(pid_t) * new_cap);
        if (!new_list) { perror("ERROR: realloc dyn pid"); return; }
        d->list = new_list;
        d->cap  = new_cap;
    }
    d->list[d->count++] = pid;
}

static pid_t spawn_replacement_lc(t_shm *shm, t_config *cfg,
                                   int floor, int global_index)
{
    t_proc_ctx ctx;
    ctx.shm        = shm;
    ctx.cfg        = cfg;
    ctx.floor      = floor;
    ctx.proc_index = 0;  

    pid_t pid = fork();
    if (pid < 0) { perror("ERROR: fork replacement_lc"); return -1; }
    if (pid == 0) {
        int lc_idx = floor * cfg->letter_carriers_per_floor;
        shm->lc_states[lc_idx].pid           = getpid();
        shm->lc_states[lc_idx].current_floor = floor;
        shm->lc_states[lc_idx].idle          = 0;

        log_fmt("[PID:%d] Letter-carrier-process_%d (replacement)"
                " initialized on floor %d\n",
                getpid(), global_index, floor);
        run_letter_carrier(&ctx);
        exit(EXIT_SUCCESS);
    }
    return pid;
}

void monitor_loop(t_shm *shm, t_config *cfg,
                  pid_t *pid_list, int pid_count)
{
    int       i, floor;
    t_pid_dyn extra = {NULL, 0, 0};
    int       lc_global_extra = cfg->num_floors * cfg->letter_carriers_per_floor;

    while (!shm->header->shutdown) {
        sem_wait(&shm->header->done_mutex);
        int done  = shm->header->done_count;
        int total = shm->header->total_words;
        sem_post(&shm->header->done_mutex);

        if (done >= total)
            break;

        for (floor = 0; floor < cfg->num_floors; floor++){
            if (count_live_lc_on_floor(shm, cfg, floor) == 0){
                log_fmt("[MONITOR] No letter-carrier on floor %d,"
                        " spawning %d replacement(s)\n",
                        floor, cfg->letter_carriers_per_floor);

                for (i = 0; i < cfg->letter_carriers_per_floor; i++){
                    pid_t pid = spawn_replacement_lc(shm, cfg, floor,
                                                     lc_global_extra++);
                    if (pid > 0)
                        dyn_push(&extra, pid);
                }
            }
        }
        usleep(10000);
    }

    log_msg("\nAll words have been transported and sorted...\n");
    shm->header->shutdown = 1;

    for (i = 0; i < pid_count; i++)
        kill(pid_list[i], SIGTERM);

    for (i = 0; i < extra.count; i++)
        kill(extra.list[i], SIGTERM);

    for (i = 0; i < pid_count; i++){
        int status;
        if (waitpid(pid_list[i], &status, 0) < 0)
            perror("waitpid");
    }
    for (i = 0; i < extra.count; i++) {
        int status;
        if (waitpid(extra.list[i], &status, 0) < 0)
            perror("waitpid");
    }
    free(extra.list);
}