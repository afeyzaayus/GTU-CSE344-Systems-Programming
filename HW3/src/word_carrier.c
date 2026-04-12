#include "../inc/process_spawn.h"
#include "../inc/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int rr_pick(t_shm *shm)
{
    int total = shm->header->total_words;
    int start, i, idx;

    sem_wait(&shm->header->rr_mutex);
    start = shm->header->rr_index;
    shm->header->rr_index = (shm->header->rr_index + 1) % total;
    sem_post(&shm->header->rr_mutex);

    for (i = 0; i < total; i++)
    {
        idx = (start + i) % total;
        if (!shm->words[idx].claimed && !shm->words[idx].admitted)
            return idx;
    }
    return -1;
}

static int try_admit(t_shm *shm, t_config *cfg, int word_idx, int arrival_floor)
{
    int sorting_floor = shm->words[word_idx].sorting_floor;
    int admitted = 0;
    int lo = arrival_floor < sorting_floor ? arrival_floor : sorting_floor;
    int hi = arrival_floor < sorting_floor ? sorting_floor : arrival_floor;

    if (lo == hi)
    {
        sem_wait(&shm->floors[lo].mutex);
        if (shm->floors[lo].current_word_count < cfg->max_words_per_floor)
        {
            shm->floors[lo].current_word_count += 2;
            admitted = 1;
        }
        sem_post(&shm->floors[lo].mutex);
    }
    else
    {
        sem_wait(&shm->floors[lo].mutex);
        sem_wait(&shm->floors[hi].mutex);
        if (shm->floors[arrival_floor].current_word_count < cfg->max_words_per_floor &&
            shm->floors[sorting_floor].current_word_count < cfg->max_words_per_floor)
        {
            shm->floors[arrival_floor].current_word_count++;
            shm->floors[sorting_floor].current_word_count++;
            admitted = 1;
        }
        sem_post(&shm->floors[hi].mutex);
        sem_post(&shm->floors[lo].mutex);
    }
    return admitted;
}

void run_word_carrier(t_proc_ctx *ctx)
{
    t_shm    *shm      = ctx->shm;
    t_config *cfg      = ctx->cfg;
    int       my_floor = ctx->floor;
    int       total    = shm->header->total_words;
    int       word_idx, j;

    while (!shm->header->shutdown)
    {
        /* FIX BUG 1: admitted_count ile kontrol et, done_count değil */
        sem_wait(&shm->header->done_mutex);
        int admitted_count = shm->header->admitted_count;
        sem_post(&shm->header->done_mutex);
        if (admitted_count >= total)
            break;

        word_idx = rr_pick(shm);
        if (word_idx < 0) { usleep(5000); continue; }

        t_word *w = &shm->words[word_idx];

        sem_wait(&shm->header->rr_mutex);
        if (w->claimed || w->admitted)
        {
            sem_post(&shm->header->rr_mutex);
            continue;
        }
        w->claimed = 1;
        sem_post(&shm->header->rr_mutex);

        log_fmt("[PID:%d] Word-carrier-process_%d claimed word %d\n",
                getpid(), ctx->proc_index, w->word_id);

        if (try_admit(shm, cfg, word_idx, my_floor))
        {
            w->arrival_floor = my_floor;
            w->admitted      = 1;
            for (j = 0; j < w->word_len; j++)
                w->tasks[j].src_floor = my_floor;

            /* FIX BUG 1: admitted_count artır */
            sem_wait(&shm->header->done_mutex);
            shm->header->admitted_count++;
            sem_post(&shm->header->done_mutex);

            log_fmt("[PID:%d] Word %d admitted to floor %d (sorting floor: %d)\n",
                    getpid(), w->word_id, my_floor, w->sorting_floor);
        }
        else
        {
            w->claimed = 0;
            log_fmt("[PID:%d] Word %d rejected (floors full), retrying later\n",
                    getpid(), w->word_id);
            usleep(5000);
        }
    }
}