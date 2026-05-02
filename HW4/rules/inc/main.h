#ifndef MAIN_H
# define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include "parser.h"
#include "shm.h"
#include "reader.h"
#include "dispatcher.h"
#include "analyzer.h"
#include "aggregator.h"
#include "watchdog.h"

extern volatile sig_atomic_t g_shutdown;

int create_pipes(int pipe_fds[][2], int count);
void close_pipe_read_ends(int pipe_fds[][2], int count);
void close_pipe_write_ends(int pipe_fds[][2], int count);

int spawn_reader_processes(t_args *args, int pipe_fds[][2], t_shm *shm,
        int *pid_count, pid_t *all_pids);
int spawn_dispatcher_process(t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count);
int spawn_analyzer_processses(t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count);
int spawn_aggregator_process(t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count);
int start_watchdog_thread(t_watchdog_args *wdog_args, t_args *args, t_shm *shm, int pipe_fds[][2], pid_t *all_pids,
        int *pid_count, pthread_t *watchdog_tid);

int fork_readers(t_args *args, t_shm *shm,
                        int pipe_fds[][2], pid_t *pids);
pid_t fork_dispatcher(t_args *args, t_shm *shm,
                              int pipe_fds[][2]);
int fork_analyzers(t_args *args, t_shm *shm,
                           int pipe_fds[][2], pid_t *pids);
pid_t fork_aggregator(t_args *args, t_shm *shm,
                              int pipe_fds[][2]);

#endif
