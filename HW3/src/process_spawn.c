#include "../inc/process_spawn.h"
#include "../inc/log.h"
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
    if (pid == 0)
    {
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
    if (pid == 0)
    {
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
    if (pid == 0)
    {
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
    if (pid == 0)
    {
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
    if (pid == 0)
    {
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

    for (floor = 0; floor < cfg->num_floors; floor++)
    {
        log_fmt("--- Initializing Floor %d ---\n", floor);
        ctx.floor = floor;

        for (i = 0; i < cfg->word_carriers_per_floor; i++)
        {
            ctx.proc_index = i;
            pid_list[idx++] = spawn_word_carrier(&ctx, wc_global++);
        }
        for (i = 0; i < cfg->letter_carriers_per_floor; i++)
        {
            ctx.proc_index = i;
            pid_list[idx++] = spawn_letter_carrier(&ctx, lc_global++);
        }
        for (i = 0; i < cfg->sorting_processes_per_floor; i++)
        {
            ctx.proc_index = i;
            pid_list[idx++] = spawn_sorting_proc(&ctx, sp_global++);
        }
    }

    pid_list[idx++] = spawn_delivery_elevator(shm, cfg);
    pid_list[idx++] = spawn_reposition_elevator(shm, cfg);
    *pid_count = idx;

    log_msg("--------------------------------------------------\n");
}

void monitor_loop(t_shm *shm, t_config *cfg,
                  pid_t *pid_list, int pid_count)
{
    int i;

    while (!shm->header->shutdown)
    {
        sem_wait(&shm->header->done_mutex);
        int done  = shm->header->done_count;
        int total = shm->header->total_words;
        sem_post(&shm->header->done_mutex);

        if (done >= total)
            break;

        usleep(10000);
    }

    log_msg("\nAll words have been transported and sorted...\n");
    shm->header->shutdown = 1;

    for (i = 0; i < pid_count; i++)
        kill(pid_list[i], SIGTERM);

    for (i = 0; i < pid_count; i++)
    {
        int status;
        if (waitpid(pid_list[i], &status, 0) < 0)
            perror("waitpid");
    }

    (void)cfg;
}