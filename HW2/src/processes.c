#include "../inc/functions.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

void parent_process(t_match *all_matches, int *total_match_count) {
    for (size_t i = 0; i < g_num_workers; i++) {
        int results[2] = {0, 0};
        read(g_pipes[i][0], results, sizeof(results));
        close(g_pipes[i][0]);
        g_worker_matches[i] = results[0];
        g_worker_files[i]   = results[1];
    }

    for (size_t i = 0; i < g_num_workers; i++) {
        t_match m;
        while (read(g_match_pipes[i][0], &m, sizeof(t_match)) == sizeof(t_match))
            if (*total_match_count < 1024)
                all_matches[(*total_match_count)++] = m;
        close(g_match_pipes[i][0]);
    }

    for (size_t i = 0; i < g_num_workers; i++)
        waitpid(g_worker_pids[i], NULL, 0);
}

int create_workers(t_args *args)
{
    int num_subdirs = root_subdirs(args->root_dir);
    t_match all_matches[1024];
    int total_match_count = 0;

    if (num_subdirs == 0) {
        printf("Notice: no subdirectories found; parent will search root directly.\n");
        int total = search_directory_collect(args->root_dir, args,
                                             all_matches, &total_match_count);
        g_num_workers = 1;
        g_worker_pids[0] = getpid();
        g_worker_matches[0] = total;
        g_worker_files[0] = total_match_count;
        print_tree(args->root_dir, all_matches, total_match_count);
        print_summary();
        return 0;
    }

    if (check_num_subdirs(num_subdirs, args) == -1)
        return 0;

    char subdirs[num_subdirs][4096];
    fill_subdirs(args->root_dir, subdirs, num_subdirs);

    g_num_workers = args->num_workers;

    setup_sigint_handler();
    listen_sigusr1();

    for (size_t i = 0; i < args->num_workers; i++) {
        pipe(g_pipes[i]);
        pipe(g_match_pipes[i]);
    }

    for (size_t i = 0; i < args->num_workers; i++) {
        g_worker_pids[i] = fork();

        if (g_worker_pids[i] == -1){
            write(STDERR_FILENO, "ERROR: fork failed\n", 19);
            return -1;
        }
        if (g_worker_pids[i] == 0) {
            ignore_child_sigint();
            close_child_pipes(args->num_workers, i);
            setup_sigterm_handler();
            set_parent_pid(getppid());
            set_worker_index(i);
            int match_count = record_match_and_file(i, num_subdirs, args, subdirs);
            notify_parent();
            exit(match_count % 256);
        }
        close(g_pipes[i][1]);
        close(g_match_pipes[i][1]);
    }

    parent_process(all_matches, &total_match_count);
    print_tree(args->root_dir, all_matches, total_match_count);
    print_summary();
    return 0;
}