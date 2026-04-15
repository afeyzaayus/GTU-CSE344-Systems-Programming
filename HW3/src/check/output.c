#include "../../inc/process_spawn.h"
#include "../../inc/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static t_shm    *g_shm       = NULL;
static t_config *g_cfg       = NULL;
static pid_t    *g_pid_list  = NULL;
static int       g_pid_count = 0;

void sigint_handler(int sig)
{
    int i;
    (void)sig;
    write(STDERR_FILENO, "\n[SIGNAL] SIGINT received, shutting down...\n", 44);

    if (g_shm)       g_shm->header->shutdown = 1;
    if (g_pid_list)  for (i = 0; i < g_pid_count; i++) kill(g_pid_list[i], SIGTERM);
    if (g_pid_list)  for (i = 0; i < g_pid_count; i++) waitpid(g_pid_list[i], NULL, 0);
    if (g_shm && g_cfg) shm_destroy(g_shm, g_cfg->num_floors);
    free(g_pid_list);
    _exit(EXIT_FAILURE);
}

void setup_signal_handler(t_shm *shm, t_config *cfg,
                           pid_t *pid_list, int pid_count)
{
    struct sigaction sa;

    g_shm = shm; g_cfg = cfg;
    g_pid_list = pid_list; g_pid_count = pid_count;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0)
    { perror("ERROR: sigaction"); exit(EXIT_FAILURE); }
}

static int cmp_words(const void *a, const void *b)
{
    const t_word *wa = (const t_word *)a;
    const t_word *wb = (const t_word *)b;
    if (wa->sorting_floor != wb->sorting_floor)
        return wa->sorting_floor - wb->sorting_floor;
    return wa->word_id - wb->word_id;
}

void write_output_file(t_shm *shm, t_config *cfg)
{
    FILE   *f;
    int     total = shm->header->total_words;
    int     i;
    t_word *sorted;

    sorted = malloc(sizeof(t_word) * total);
    if (!sorted) { perror("ERROR: malloc sorted"); return; }
    memcpy(sorted, shm->words, sizeof(t_word) * total);
    qsort(sorted, total, sizeof(t_word), cmp_words);

    f = fopen(cfg->output_file, "w");
    if (!f) { perror("ERROR: fopen output"); free(sorted); return; }

    for (i = 0; i < total; i++)
        fprintf(f, "%d %s %d\n",
                sorted[i].word_id, sorted[i].original, sorted[i].sorting_floor);

    fclose(f);
    free(sorted);

    log_msg("Output file is being created...\n");
    log_fmt("Output written to '%s'\n", cfg->output_file);
}

void print_summary(t_shm *shm)
{
    int total        = shm->header->total_words;
    int completed    = shm->header->done_count;
    int retries      = shm->header->retry_count;
    int delivery_ops = shm->header->delivery_ops;
    int repos_ops    = shm->header->reposition_ops;
    int total_chars  = 0;
    int i, j;

    for (i = 0; i < total; i++)
        total_chars += shm->words[i].word_len;

    log_msg("\nSystem Summary:\n");
    log_msg("--------------------------------------------------\n");
    log_fmt("Total words:                  %d\n", total);
    log_fmt("Completed words:              %d\n", completed);
    log_fmt("Retries: %d\n", retries);
    log_fmt("Characters transported:       %d\n", total_chars);
    log_fmt("Delivery elevator operations: %d\n", delivery_ops);
    log_fmt("Reposition elevator operations: %d\n", repos_ops);

    log_msg("\nPer-word statistics:\n");
    for (i = 0; i < total; i++){
        t_word *w           = &shm->words[i];
        int     delivered   = 0;

        for (j = 0; j < w->word_len; j++)
            if (w->tasks[j].delivered)
                delivered++;

        log_fmt("  Word %3d %-*s | arrival: %2d | sorting: %2d"
                " | chars: %d/%d | %s\n",
                w->word_id,
                MAX_WORD_LEN, w->original,
                w->arrival_floor,
                w->sorting_floor,
                delivered, w->word_len,
                w->completed ? "COMPLETED" : "INCOMPLETE");
    }

    log_msg("\nProgram terminated successfully.\n");
}