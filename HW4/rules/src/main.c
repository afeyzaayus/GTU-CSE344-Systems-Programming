#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include "argument_parsing.h"
#include "shm.h"
#include "reader.h"
#include "dispatcher.h"
#include "analyzer.h"
#include "aggregator.h"
#include "watchdog.h"

// --------------------------------------------------
// SIGINT HANDLER — sadece async-signal-safe işlemler
// printf/malloc/free YASAK burada
// --------------------------------------------------
volatile sig_atomic_t g_shutdown = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    g_shutdown = 1;
}

// --------------------------------------------------
// PIPE YARDIMCILARI
// Her Reader process için bir pipe çifti:
//   pipe_fds[i][0] = read end  (parent/watchdog okur)
//   pipe_fds[i][1] = write end (reader yazar)
// --------------------------------------------------
static int create_pipes(int pipe_fds[][2], int count)
{
    for (int i = 0; i < count; i++)
    {
        if (pipe(pipe_fds[i]) == -1)
        {
            perror("pipe");
            return (0);
        }
    }
    return (1);
}

// Child process'te write end açık kalır, read end'leri kapat
static void close_pipe_read_ends(int pipe_fds[][2], int count)
{
    for (int i = 0; i < count; i++)
        close(pipe_fds[i][0]);
}

// Parent/watchdog'da read end'ler açık kalır, write end'leri kapat
static void close_pipe_write_ends(int pipe_fds[][2], int count)
{
    for (int i = 0; i < count; i++)
        close(pipe_fds[i][1]);
}

// --------------------------------------------------
// FORK: READER PROCESS'LER
// Her log dosyası için bir Reader process forklanır.
// --------------------------------------------------
static int fork_readers(t_args *args, t_shm *shm,
                        int pipe_fds[][2], pid_t *pids)
{
    for (int i = 0; i < args->file_count; i++)
    {
        printf("[PID:%d] Forking Reader %d -> %s\n",
               getpid(), i, args->log_files[i]);

        pid_t pid = fork();
        if (pid < 0)
            return (perror("fork reader"), 0);

        if (pid == 0)
        {
            // -- CHILD: Reader process --

            // Diğer reader'ların write end'lerini kapat
            // (sadece kendi write end'imizi kullanacağız)
            for (int j = 0; j < args->file_count; j++)
            {
                if (j != i)
                    close(pipe_fds[j][1]);
                close(pipe_fds[j][0]); // read end'ler hep kapatılır
            }

            // Reader process'i çalıştır
            // reader_index ve pipe write end'i geçiyoruz
            run_reader(args, shm, i, pipe_fds[i][1]);

            // Kendi write end'ini kapat ve çık
            close(pipe_fds[i][1]);
            exit(0);
        }

        // -- PARENT: pid'i kaydet --
        pids[i] = pid;
    }
    return (1);
}

// --------------------------------------------------
// FORK: DISPATCHER PROCESS
// --------------------------------------------------
static pid_t fork_dispatcher(t_args *args, t_shm *shm,
                              int pipe_fds[][2])
{
    printf("[PID:%d] Forking Dispatcher\n", getpid());

    pid_t pid = fork();
    if (pid < 0)
        return (perror("fork dispatcher"), -1);

    if (pid == 0)
    {
        // -- CHILD: Dispatcher process --
        // Dispatcher pipe kullanmıyor, hepsini kapat
        close_pipe_read_ends(pipe_fds, args->file_count);
        close_pipe_write_ends(pipe_fds, args->file_count);

        run_dispatcher(args, shm);
        exit(0);
    }

    return (pid);
}

// --------------------------------------------------
// FORK: ANALYZER PROCESS'LER (×4, her level için)
// --------------------------------------------------
static int fork_analyzers(t_args *args, t_shm *shm,
                           int pipe_fds[][2], pid_t *pids)
{
    const char *level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};

    for (int i = 0; i < 4; i++)
    {
        printf("[PID:%d] Forking Analyzer %s (index %d)\n",
               getpid(), level_names[i], i);

        pid_t pid = fork();
        if (pid < 0)
            return (perror("fork analyzer"), 0);

        if (pid == 0)
        {
            // -- CHILD: Analyzer process --
            close_pipe_read_ends(pipe_fds, args->file_count);
            close_pipe_write_ends(pipe_fds, args->file_count);

            // i = level index (0=ERROR, 1=WARN, 2=INFO, 3=DEBUG)
            run_analyzer(args, shm, i);
            exit(0);
        }

        pids[i] = pid;
    }
    return (1);
}

// --------------------------------------------------
// FORK: AGGREGATOR PROCESS
// --------------------------------------------------
static pid_t fork_aggregator(t_args *args, t_shm *shm,
                              int pipe_fds[][2])
{
    printf("[PID:%d] Forking Aggregator\n", getpid());

    pid_t pid = fork();
    if (pid < 0)
        return (perror("fork aggregator"), -1);

    if (pid == 0)
    {
        // -- CHILD: Aggregator process --
        close_pipe_read_ends(pipe_fds, args->file_count);
        close_pipe_write_ends(pipe_fds, args->file_count);

        run_aggregator(args, shm);
        exit(0);
    }

    return (pid);
}

// --------------------------------------------------
// WAITPID LOOP — tüm child'lar bitene kadar bekle
// SIGINT gelirse child'lara SIGTERM gönder
// --------------------------------------------------
static void wait_for_children(pid_t *all_pids, int count)
{
    int     remaining = count;
    int     status;
    pid_t   wpid;

    while (remaining > 0)
    {
        // SIGINT geldi mi kontrol et
        if (g_shutdown)
        {
            // Tüm child'lara SIGTERM gönder
            for (int i = 0; i < count; i++)
            {
                if (all_pids[i] > 0)
                    kill(all_pids[i], SIGTERM);
            }

            // 5 saniyelik deadline ile bekle
            int deadline = 50; // 50 × 100ms = 5s
            while (deadline-- > 0 && remaining > 0)
            {
                wpid = waitpid(-1, &status, WNOHANG);
                if (wpid > 0)
                {
                    remaining--;
                    // pid'i sıfırla
                    for (int i = 0; i < count; i++)
                        if (all_pids[i] == wpid)
                            all_pids[i] = -1;
                }
                usleep(100000); // 100ms
            }
            break;
        }

        // Normal bekleme
        wpid = waitpid(-1, &status, 0);
        if (wpid > 0)
        {
            remaining--;
            for (int i = 0; i < count; i++)
                if (all_pids[i] == wpid)
                    all_pids[i] = -1;
        }
        else if (wpid == -1 && errno == EINTR)
            continue; // sinyal ile kesildi, döngüye dön
        else
            break;
    }
}

// --------------------------------------------------
// FINAL SUMMARY — tüm child'lar bittikten sonra
// --------------------------------------------------
static void print_final_summary(t_args *args, t_shm *shm)
{
    long    total_entries = 0;
    double  total_weighted = 0.0;
    double  high_priority = 0.0;

    const char *level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};

    for (int i = 0; i < 4; i++)
    {
        total_entries += shm->c->results[i].total_entries;
        total_weighted += shm->c->results[i].total_weighted_score;
    }

    // Region D'den high priority score hesapla
    // (Aggregator bunu zaten Region C'ye yazmış olmalı,
    //  ama basit tutmak için Region C'den okuyoruz)
    // NOT: Aggregator high_priority_score'u Region C'ye yazmıyorsa
    // bu değer 0 kalır — Aggregator implementasyonunda düzeltilecek
    high_priority = shm->c->high_priority_score;

    printf("==================================================\n");
    printf("SYSTEM SUMMARY\n");

    // Keywords
    printf("Keywords   : ");
    for (int i = 0; i < args->keyword_count; i++)
    {
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

    for (int i = 0; i < 4; i++)
    {
        printf("  %-5s : %ld entries, score: %.1f\n",
               level_names[i],
               shm->c->results[i].total_entries,
               shm->c->results[i].total_weighted_score);
    }

    printf("==================================================\n");
    printf("Program terminated successfully.\n");
}

int setup_signal_handler(struct sigaction *sa)
{
    memset(sa, 0, sizeof(*sa)); 
    sa->sa_handler = sigint_handler;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = 0;
    if (sigaction(SIGINT, sa, NULL) == -1)
        return (perror("sigaction"), 0);
    return (1);
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

int main(int argc, char **argv)
{
    t_args      args;
    struct sigaction sa;
    t_shm       shm;
    pthread_t   watchdog_tid;
    int pipe_fds[MAX_FILES][2]; // pipe_fds: her reader için bir pipe çifti
    pid_t all_pids[MAX_FILES + 6]; // reader(×N) + dispatcher(1) + analyzer(4) + aggregator(1)
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

    // 12. WATCHDOG'U DURDUR
    g_shutdown = 1;
    pthread_join(watchdog_tid, NULL);

    // 13. PIPE READ END'LERİ KAPAT
    close_pipe_read_ends(pipe_fds, args.file_count);

    // 14. FINAL SUMMARY
    print_final_summary(&args, &shm);

    // 15. CLEANUP
    free_args(&args);
    shm_destroy(&shm, &args);

    return (0);
}