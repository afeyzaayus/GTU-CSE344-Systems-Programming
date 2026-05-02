#include "../../inc/main.h"

int fork_readers(t_args *args, t_shm *shm,
                        int pipe_fds[][2], pid_t *pids)
{
    for (int i = 0; i < args->file_count; i++) {
        printf("[PID:%d] Forking Reader %d -> %s\n",
               getpid(), i, args->log_files[i]);

        pid_t pid = fork();
        if (pid < 0)
            return (perror("fork reader"), 0);

        if (pid == 0) {
            for (int j = 0; j < args->file_count; j++){
                if (j != i)
                    close(pipe_fds[j][1]);
                close(pipe_fds[j][0]); 
            }
            run_reader(args, shm, i, pipe_fds[i][1]);
            close(pipe_fds[i][1]);
            exit(0);
        }
        pids[i] = pid;
    }
    return (1);
}

pid_t fork_dispatcher(t_args *args, t_shm *shm,
                              int pipe_fds[][2])
{
    printf("[PID:%d] Forking Dispatcher\n", getpid());

    pid_t pid = fork();
    if (pid < 0)
        return (perror("fork dispatcher"), -1);

    if (pid == 0) {
        close_pipe_read_ends(pipe_fds, args->file_count);
        close_pipe_write_ends(pipe_fds, args->file_count);

        run_dispatcher(args, shm);
        exit(0);
    }

    return (pid);
}

int fork_analyzers(t_args *args, t_shm *shm,
                           int pipe_fds[][2], pid_t *pids)
{
    const char *level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};

    for (int i = 0; i < 4; i++) {
        printf("[PID:%d] Forking Analyzer %s (index %d)\n",
               getpid(), level_names[i], i);

        pid_t pid = fork();
        if (pid < 0)
            return (perror("fork analyzer"), 0);

        if (pid == 0) {
            close_pipe_read_ends(pipe_fds, args->file_count);
            close_pipe_write_ends(pipe_fds, args->file_count);

            run_analyzer(args, shm, i);
            exit(0);
        }
        pids[i] = pid;
    }
    return (1);
}

pid_t fork_aggregator(t_args *args, t_shm *shm,
                              int pipe_fds[][2])
{
    printf("[PID:%d] Forking Aggregator\n", getpid());

    pid_t pid = fork();
    if (pid < 0)
        return (perror("fork aggregator"), -1);

    if (pid == 0){
        close_pipe_read_ends(pipe_fds, args->file_count);
        close_pipe_write_ends(pipe_fds, args->file_count);

        run_aggregator(args, shm);
        exit(0);
    }

    return (pid);
}
