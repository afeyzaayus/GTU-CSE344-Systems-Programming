#include "../inc/process_spawn.h"
#include "../inc/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int sort_step(t_shm *shm, int word_idx, int floor)
{
    t_word *w = &shm->words[word_idx];
    int changed = 0, i, j, all_fixed;

    if (w->sorting_floor != floor || !w->admitted || w->completed)
        return 0;
    if (sem_trywait(&w->word_mutex) != 0)
        return 0;

    for (i = 0; i < w->word_len; i++)
    {
        if (!w->occupied[i] || w->fixed[i])
            continue;

        char c = w->sorting_area[i];
        int correct = -1;

        for (j = 0; j < w->word_len; j++)
        {
            if (w->original[j] == c && !w->fixed[j])
            { correct = j; break; }
        }
        if (correct < 0) continue;

        if (correct == i)
        {
            w->fixed[i] = 1;
            log_fmt("[PID:%d] Sorting-process fixed char '%c' of word %d at index %d\n",
                    getpid(), c, w->word_id, i);
            changed = 1;
        }
        else if (!w->occupied[correct])
        {
            w->sorting_area[correct] = c;
            w->occupied[correct]     = 1;
            w->sorting_area[i]       = '\0';
            w->occupied[i]           = 0;
            log_fmt("[PID:%d] Sorting-process moved char '%c' of word %d: %d -> %d\n",
                    getpid(), c, w->word_id, i, correct);
            changed = 1;
        }
        else if (!w->fixed[correct])
        {
            char tmp                 = w->sorting_area[correct];
            w->sorting_area[correct] = c;
            w->sorting_area[i]       = tmp;
            log_fmt("[PID:%d] Sorting-process swapped chars '%c'<->'%c' of word %d\n",
                    getpid(), c, tmp, w->word_id);
            changed = 1;
        }
    }

    /* FIX BUG 2: tüm karakterler teslim edildi mi önce kontrol et */
    int all_delivered = 1;
    for (i = 0; i < w->word_len; i++)
        if (!w->tasks[i].delivered) { all_delivered = 0; break; }

    all_fixed = 1;
    for (i = 0; i < w->word_len; i++)
        if (!w->fixed[i]) { all_fixed = 0; break; }

    if (all_delivered && all_fixed && !w->completed)
    {
        w->completed = 1;
        sem_wait(&shm->floors[floor].mutex);
        shm->floors[floor].current_word_count--;
        sem_post(&shm->floors[floor].mutex);
        sem_wait(&shm->header->done_mutex);
        shm->header->done_count++;
        sem_post(&shm->header->done_mutex);
        log_fmt("[PID:%d] Word %d COMPLETED on floor %d\n",
                getpid(), w->word_id, floor);
    }

    sem_post(&w->word_mutex);
    return changed;
}

void run_sorting_proc(t_proc_ctx *ctx)
{
    t_shm *shm = ctx->shm;
    int my_floor = ctx->floor;
    int total = shm->header->total_words;
    int w, active;

    while (!shm->header->shutdown)
    {
        active = 0;
        for (w = 0; w < total; w++)
            active += sort_step(shm, w, my_floor);
        if (!active)
            usleep(10000);
    }
}