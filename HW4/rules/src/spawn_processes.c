#include "main.h"

int create_pipes(int pipe_fds[][2], int count) {
    for (int i = 0; i < count; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("pipe");
            return (0);
        }
    }
    return (1);
}

void close_pipe_read_ends(int pipe_fds[][2], int count) {
    for (int i = 0; i < count; i++)
        close(pipe_fds[i][0]);
}

void close_pipe_write_ends(int pipe_fds[][2], int count) {
    for (int i = 0; i < count; i++)
        close(pipe_fds[i][1]);
}

int spawn_reader_processes(t_args *args, int pipe_fds[][2], t_shm *shm,
        int *pid_count, pid_t *all_pids){ 
    pid_t reader_pids[MAX_FILES];

    if (!create_pipes(pipe_fds, args->file_count))
        return (shm_destroy(shm, args), 0);

    if (!fork_readers(args, shm, pipe_fds, reader_pids))
        return (shm_destroy(shm, args), 0);

    for (int i = 0; i < args->file_count; i++)
        all_pids[(*pid_count)++] = reader_pids[i];

    return 1;
}

int spawn_dispatcher_process(t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count){
    pid_t disp_pid = fork_dispatcher(args, shm, pipe_fds);
    if (disp_pid < 0)
        return (shm_destroy(shm, args), 0);
    all_pids[(*pid_count)++] = disp_pid;
    return 1;
}

int spawn_analyzer_processses(t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count){
    pid_t analyzer_pids[4];
    if (!fork_analyzers(args, shm, pipe_fds, analyzer_pids))
        return (shm_destroy(shm, args), 0);

    for (int i = 0; i < 4; i++)
        all_pids[(*pid_count)++] = analyzer_pids[i];

    return 1;
}

int spawn_aggregator_process(t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count){
    pid_t agg_pid = fork_aggregator(args, shm, pipe_fds);
    if (agg_pid < 0)
        return (shm_destroy(shm, args), 0);
    all_pids[(*pid_count)++] = agg_pid;
    return 1;
}

int start_watchdog_thread(t_watchdog_args *wdog_args, t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count, pthread_t *watchdog_tid){
    // bundan dolayı sorun çıkabilir
    wdog_args->pipe_fds      = pipe_fds;
    wdog_args->reader_count  = args->file_count;
    wdog_args->log_files     = args->log_files;
    wdog_args->all_pids      = all_pids;
    wdog_args->pid_count     = *pid_count;
    wdog_args->shutdown      = &g_shutdown;

    if (pthread_create(watchdog_tid, NULL, watchdog_thread, wdog_args) != 0)
        return (perror("pthread_create watchdog"), shm_destroy(shm, args), 0);
    printf("[PID:%d] Watchdog thread started.\n", getpid());
    return 1;
}