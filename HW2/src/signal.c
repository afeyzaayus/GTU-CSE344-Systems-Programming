// signal.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../inc/functions.h"

pid_t        g_parent_pid        = 0;
pid_t        g_worker_pids[8];
int          g_num_workers       = 0;
int          g_worker_index      = -1;
int          g_pipes[8][2];

volatile sig_atomic_t g_partial_matches   = 0;
volatile sig_atomic_t g_files_scanned     = 0;
volatile sig_atomic_t g_sigusr1_count     = 0;
volatile sig_atomic_t g_worker_matches[8] = {0};
volatile sig_atomic_t g_worker_files[8]   = {0};
volatile sig_atomic_t g_sigterm_sent[8]   = {0};
int g_match_pipes[8][2];  // match sonuçları için ayrı pipe
int g_match_pipe_fd = -1; // child'ın yazma ucu

static int append_str(char *buf, int pos, const char *s)
{
    while (*s)
        buf[pos++] = *s++;
    return pos;
}

int safe_int_to_str(int n, char *str)
{
    int i = 0;
    if (n == 0) {
        str[i++] = '0';
    } else {
        while (n > 0) {
            str[i++] = (n % 10) + '0';
            n /= 10;
        }
    }
    str[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char tmp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = tmp;
    }
    return i;
}

void print_summary(void)
{
    int total_matches = 0;
    int total_files   = 0;

    for (int i = 0; i < g_num_workers; i++) {
        total_matches += g_worker_matches[i];
        total_files   += g_worker_files[i];
    }

    char buf[512];
    char num[16];
    int len;

    write(STDOUT_FILENO, "\n--- Summary ---\n", 17);

    // Total workers used
    len = 0;
    len = append_str(buf, len, "Total workers used  : ");
    safe_int_to_str(g_num_workers, num);
    len = append_str(buf, len, num);
    len = append_str(buf, len, "\n");
    write(STDOUT_FILENO, buf, len);

    // Total files scanned
    len = 0;
    len = append_str(buf, len, "Total files scanned : ");
    safe_int_to_str(total_files, num);
    len = append_str(buf, len, num);
    len = append_str(buf, len, "\n");
    write(STDOUT_FILENO, buf, len);

    // Total matches found
    len = 0;
    len = append_str(buf, len, "Total matches found : ");
    safe_int_to_str(total_matches, num);
    len = append_str(buf, len, num);
    len = append_str(buf, len, "\n");
    write(STDOUT_FILENO, buf, len);

    // Per-worker
    for (int i = 0; i < g_num_workers; i++)
    {
        char pid_str[16], match_str[16];
        safe_int_to_str((int)g_worker_pids[i], pid_str);
        safe_int_to_str((int)g_worker_matches[i], match_str);

        len = 0;
        len = append_str(buf, len, "Worker PID ");
        len = append_str(buf, len, pid_str);
        len = append_str(buf, len, " : ");
        len = append_str(buf, len, match_str);
        len = append_str(buf, len, (g_worker_matches[i] == 1) ? " match\n" : " matches\n");
        write(STDOUT_FILENO, buf, len);
    }
}



void sigchld_handler(int sig)
{
    (void)sig;

    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        int expected = 0;
        for (int i = 0; i < g_num_workers; i++)
        {
            if (g_worker_pids[i] == pid && g_sigterm_sent[i])
            {
                expected = 1;
                break;
            }
        }

        if (!expected)
        {
            char pid_str[16], status_str[16], buf[128];
            int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            safe_int_to_str((int)pid, pid_str);
            safe_int_to_str(exit_status, status_str);

            int len = 0;
            len = append_str(buf, len, "[Parent] Worker PID:");
            len = append_str(buf, len, pid_str);
            len = append_str(buf, len, " terminated unexpectedly (exit status: ");
            len = append_str(buf, len, status_str);
            len = append_str(buf, len, ").\n");
            write(STDERR_FILENO, buf, len);
        }
    }
}

void setup_sigchld_handler(void)
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

void sigint_handler(int sig)
{
    (void)sig;

    write(STDOUT_FILENO, "[Parent] SIGINT received. Terminating all workers...\n", 53);

    for (int i = 0; i < g_num_workers; i++)
    {
        if (g_worker_pids[i] > 0)
        {
            g_sigterm_sent[i] = 1;
            kill(g_worker_pids[i], SIGTERM);
        }
    }

    for (int i = 0; i < g_num_workers; i++)
    {
        int results[2] = {0, 0};
        read(g_pipes[i][0], results, sizeof(results));
        close(g_pipes[i][0]);
        g_worker_matches[i] = results[0];
        g_worker_files[i]   = results[1];
    }

    sleep(3);

    for (int i = 0; i < g_num_workers; i++)
        if (g_worker_pids[i] > 0)
            kill(g_worker_pids[i], SIGKILL);

    for (int i = 0; i < g_num_workers; i++)
        waitpid(g_worker_pids[i], NULL, WNOHANG);

    print_summary();
    _exit(1);
}
void setup_sigint_handler(void)
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
}



void sigterm_handler(int sig)
{
    (void)sig;

    // Pipe'a yaz
    if (g_worker_index >= 0 && g_worker_index < 8)
    {
        int results[2] = {(int)g_partial_matches, (int)g_files_scanned};
        write(g_pipes[g_worker_index][1], results, sizeof(results));
        close(g_pipes[g_worker_index][1]);
    }

    char pid_str[16], match_str[16], file_str[16], buf[256];
    safe_int_to_str((int)getpid(),          pid_str);
    safe_int_to_str((int)g_partial_matches, match_str);
    safe_int_to_str((int)g_files_scanned,   file_str);

    int len = 0;
    len = append_str(buf, len, "[Worker PID:");
    len = append_str(buf, len, pid_str);
    len = append_str(buf, len, "] SIGTERM received. Partial matches: ");
    len = append_str(buf, len, match_str);
    len = append_str(buf, len, " (");
    len = append_str(buf, len, file_str);
    len = append_str(buf, len, " files). Exiting.\n");
    write(STDOUT_FILENO, buf, len);

    _exit((int)g_partial_matches % 256);
}

void setup_sigterm_handler(void)
{
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
}

void sigusr1_handler(int sig) { (void)sig; g_sigusr1_count++; }

void notify_parent(void)
{
    if (g_parent_pid > 0)
        kill(g_parent_pid, SIGUSR1);
}

void set_parent_pid(pid_t p)   { g_parent_pid   = p; }
void set_worker_index(int idx) { g_worker_index = idx; }

void record_match(void)
{
    if (g_worker_index >= 0 && g_worker_index < 8)
        g_worker_matches[g_worker_index]++;
}