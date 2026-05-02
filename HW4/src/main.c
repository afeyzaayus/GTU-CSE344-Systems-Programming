#include "../inc/main.h"

volatile sig_atomic_t g_shutdown = 0;


static void sigint_handler(int sig) {
    (void)sig;
    g_shutdown = 1;
}

static void wait_for_children(pid_t *all_pids, int count){
    int     remaining = count;
    int     status;
    pid_t   wpid;

    while (remaining > 0) {
        if (g_shutdown){
            fprintf(stderr, "SIGINT received, SIGTERM is being sent to %d children.\n", count);
            for (int i = 0; i < count; i++){
                if (all_pids[i] > 0){
                    kill(all_pids[i], SIGTERM);
                }
            }
            int deadline = 50;
            while (deadline-- > 0 && remaining > 0){
                wpid = waitpid(-1, &status, WNOHANG);
                if (wpid > 0){
                    remaining--;
                    for (int i = 0; i < count; i++)
                        if (all_pids[i] == wpid)
                            all_pids[i] = -1;
                }
                usleep(100000);
            }
            break;
        }

        wpid = waitpid(-1, &status, WNOHANG);
        if (wpid > 0){
            remaining--;
            for (int i = 0; i < count; i++)
                if (all_pids[i] == wpid)
                    all_pids[i] = -1;
        }
        else if (wpid == 0)
            usleep(50000);
        else if (wpid == -1 && errno == EINTR)
            continue;
        else
            break;
    }
}

static void print_final_summary(t_args *args, t_shm *shm){
    long    total_entries = 0;
    double  total_weighted = 0.0;
    double  high_priority = 0.0;

    const char *level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};

    for (int i = 0; i < 4; i++){
        total_entries += shm->c->results[i].total_entries;
        total_weighted += shm->c->results[i].total_weighted_score;
    }

    high_priority = shm->c->high_priority_score;
    printf("==================================================\n");
    printf("SYSTEM SUMMARY\n");
    printf("Keywords   : ");
    for (int i = 0; i < args->keyword_count; i++){
        printf("%s", args->keywords[i]);
        if (i < args->keyword_count - 1)
            printf(", ");
    }
    printf("\n");
    printf("Log files  : %d\n", args->file_count);
    printf("Total entries : %ld\n", total_entries);
    printf("Total weighted: %.1f\n", total_weighted);
    printf("High-priority : %.1f (source filter: %s)\n",
           high_priority, args->filter_file);

    for (int i = 0; i < 4; i++) {
        printf("  %-5s : %ld entries, score: %.1f\n",
               level_names[i],
               shm->c->results[i].total_entries,
               shm->c->results[i].total_weighted_score);
    }

    printf("==================================================\n");
    printf("Program terminated successfully.\n");
}

int setup_signal_handler(struct sigaction *sa) {
    memset(sa, 0, sizeof(*sa)); 
    sa->sa_handler = sigint_handler;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = SA_RESTART;
    if (sigaction(SIGINT, sa, NULL) == -1)
        return (perror("sigaction"), 0);
    return (1);
}

int main(int argc, char **argv){
    t_args      args;
    struct sigaction sa;
    t_shm       shm;
    pthread_t   watchdog_tid;
    int pipe_fds[MAX_FILES][2]; 
    pid_t all_pids[MAX_FILES + 6]; 
    int   pid_count = 0;
    t_watchdog_args wdog_args;
    

    if (!start_parsing(argc, argv, &args))
        return (1);

    memset(&shm, 0, sizeof(t_shm));
    memset(all_pids, -1, sizeof(all_pids));

    printf("[PID:%d] Parent started. Files: %d, Keywords: ",
           getpid(), args.file_count);
    for (int i = 0; i < args.keyword_count; i++) {
        printf("%s", args.keywords[i]);
        if (i < args.keyword_count - 1)
        printf(",");
    }
    printf("\n");
    
    if(!setup_signal_handler(&sa))
        return 1;
    
    if (!shm_init(&shm, &args))
        return (fprintf(stderr, "Error: shm_init failed\n"), 1);
    
    printf("[PID:%d] Shared memory initialized (A:%d B:%dx4 D:%d).\n",
        getpid(), args.cap_a, args.cap_b, args.cap_d);
        
    if (!spawn_reader_processes(&args, pipe_fds, &shm, &pid_count, all_pids))
        return 1;
    if (!spawn_dispatcher_process(&args, &shm, pipe_fds, all_pids, &pid_count))
        return 1;
    if(!spawn_analyzer_processses(&args, &shm, pipe_fds, all_pids, &pid_count))
        return 1;
    if(!spawn_aggregator_process(&args, &shm, pipe_fds, all_pids, &pid_count))
        return 1;

    close_pipe_write_ends(pipe_fds, args.file_count);

    if (!start_watchdog_thread(&wdog_args, &args, &shm, pipe_fds, all_pids, &pid_count, &watchdog_tid))
        return 1;

    wait_for_children(all_pids, pid_count);
    g_shutdown = 1;
    pthread_join(watchdog_tid, NULL);
    close_pipe_read_ends(pipe_fds, args.file_count);
    print_final_summary(&args, &shm);
    free_args(&args);
    shm_destroy(&shm, &args);

    return (0);
}