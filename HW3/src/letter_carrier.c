#include "../inc/process_spawn.h"
#include "../inc/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int find_task(t_shm *shm, int floor, int *out_word, int *out_task)
{
    int w, t, total = shm->header->total_words;

    for (w = 0; w < total; w++)
    {
        t_word *word = &shm->words[w];
        if (!word->admitted || word->completed)
            continue;

        if (word->arrival_floor != floor)
            continue;

        for (t = 0; t < word->word_len; t++){
            if (word->tasks[t].claimed || word->tasks[t].delivered)
                continue;
            if (word->tasks[t].src_floor != floor)
                continue;

            sem_wait(&word->word_mutex);
            if (!word->tasks[t].claimed && !word->tasks[t].delivered){
                word->tasks[t].claimed = 1;
                sem_post(&word->word_mutex);
                *out_word = w;
                *out_task = t;
                return 1;
            }
            sem_post(&word->word_mutex);
        }
    }
    return 0;
}

static void place_char(t_shm *shm, int word_idx, int task_idx)
{
    t_word *w = &shm->words[word_idx];
    int slot = -1, i;

    sem_wait(&w->word_mutex);
    for (i = 0; i < w->word_len; i++){
        if (!w->occupied[i] && !w->fixed[i])
        { slot = i; break; }
    }
    if (slot >= 0){
        w->sorting_area[slot]        = w->tasks[task_idx].character;
        w->occupied[slot]            = 1;
        w->tasks[task_idx].delivered = 1;
    }
    sem_post(&w->word_mutex);

    if (slot >= 0)
        log_fmt("[PID:%d] Letter-carrier placed char '%c' of word %d into slot %d\n",
                getpid(), w->tasks[task_idx].character, w->word_id, slot);
    else
        log_err("[PID:%d] ERROR: No free slot for char '%c' of word %d\n",
                getpid(), w->tasks[task_idx].character, w->word_id);
}

static void request_delivery(t_shm *shm, int src_floor, int dest_floor,
                              int word_idx, int task_idx)
{
    t_elevator *elev = shm->delivery;

    log_fmt("[PID:%d] Letter-carrier requested delivery elevator"
            " from floor %d to floor %d\n", getpid(), src_floor, dest_floor);

    sem_wait(&elev->mutex);
    shm->delivery_requests[dest_floor]++;
    elev->current_load++;
    sem_post(&elev->mutex);
    sem_post(&elev->request_sem);

    while (!shm->header->shutdown){
        sem_wait(&elev->mutex);
        int arrived = (shm->delivery_requests[dest_floor] == 0
                       && elev->current_floor == dest_floor);
        sem_post(&elev->mutex);
        if (arrived) break;
        usleep(5000);
    }

    sem_wait(&elev->mutex);
    elev->current_load--;
    sem_post(&elev->mutex);

    log_fmt("[PID:%d] Letter-carrier brought char '%c' of word %d to floor %d\n",
            getpid(), shm->words[word_idx].tasks[task_idx].character,
            shm->words[word_idx].word_id, dest_floor);
}

static int request_reposition(t_shm *shm, t_config *cfg, int current_floor)
{
    t_elevator *elev = shm->reposition;
    int dest = rand() % cfg->num_floors;
    if (dest == current_floor)
        dest = (dest + 1) % cfg->num_floors;

    log_fmt("[PID:%d] Letter-carrier requested reposition elevator from floor %d\n",
            getpid(), current_floor);

    sem_wait(&elev->mutex);
    shm->reposition_requests[dest]++;
    elev->current_load++;
    sem_post(&elev->mutex);
    sem_post(&elev->request_sem);

    while (!shm->header->shutdown){
        sem_wait(&elev->mutex);
        int arrived = (shm->reposition_requests[dest] == 0
                       && elev->current_floor == dest);
        sem_post(&elev->mutex);
        if (arrived) break;
        usleep(5000);
    }

    sem_wait(&elev->mutex);
    elev->current_load--;
    sem_post(&elev->mutex);

    log_fmt("[PID:%d] Letter-carrier resumed work on floor %d\n", getpid(), dest);
    return dest;
}

void run_letter_carrier(t_proc_ctx *ctx)
{
    t_shm    *shm      = ctx->shm;
    t_config *cfg      = ctx->cfg;
    int       my_floor = ctx->floor;
    int       word_idx, task_idx, idle_ticks = 0;

    srand(getpid());

    while (!shm->header->shutdown){
        if (!find_task(shm, my_floor, &word_idx, &task_idx)){
            if (++idle_ticks >= 5){
                idle_ticks = 0;
                my_floor = request_reposition(shm, cfg, my_floor);
            }
            else
                usleep(10000);
            continue;
        }

        idle_ticks = 0;
        t_word      *w    = &shm->words[word_idx];
        t_char_task *task = &w->tasks[task_idx];

        log_fmt("[PID:%d] Letter-carrier selected char '%c' of word %d from floor %d\n",
                getpid(), task->character, w->word_id, my_floor);

        if (task->dest_floor == my_floor){
            log_fmt("[PID:%d] Destination is same floor -> direct placement\n", getpid());
            place_char(shm, word_idx, task_idx);
        }
        else{
            request_delivery(shm, my_floor, task->dest_floor, word_idx, task_idx);
            my_floor = task->dest_floor;
            place_char(shm, word_idx, task_idx);
        }
    }
}