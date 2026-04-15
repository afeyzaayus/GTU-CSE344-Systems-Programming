#include "../inc/shm.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void init_shm_header_values(t_shm_header *header, int word_count)
{
    header->total_words = word_count;
    header->rr_index    = 0;
    header->done_count  = 0;
    header->shutdown    = 0;

    if (sem_init(&header->rr_mutex,   1, 1) < 0) { perror("ERROR: sem_init rr");   exit(EXIT_FAILURE); }
    if (sem_init(&header->done_mutex, 1, 1) < 0) { perror("ERROR: sem_init done"); exit(EXIT_FAILURE); }
}

void init_word_info(t_shm *shm, int word_count, t_line_input *words_input){
    for (int i = 0; i < word_count; i++){
        t_word *w = &shm->words[i];

        w->word_id      = words_input[i].word_id;
        w->sorting_floor = words_input[i].sorting_floor;
        w->arrival_floor = -1; 
        w->word_len     = (int)strlen(words_input[i].word);

        strncpy(w->original, words_input[i].word, MAX_WORD_LEN - 1);
        w->original[MAX_WORD_LEN - 1] = '\0';

        for (int j = 0; j < w->word_len; j++) {
            w->tasks[j].word_id        = w->word_id;
            w->tasks[j].character      = w->original[j];
            w->tasks[j].original_index = j;
            w->tasks[j].src_floor      = -1;
            w->tasks[j].dest_floor     = w->sorting_floor;
            w->tasks[j].claimed        = 0;
            w->tasks[j].delivered      = 0;
        }
        if (sem_init(&w->word_mutex, 1, 1) < 0){
            perror("ERROR: sem_init word_mutex");
            exit(EXIT_FAILURE);
        }
    }
}

void init_floors(t_shm *shm, int num_floors){
    for (int i = 0; i < num_floors; i++){
        shm->floors[i].current_word_count = 0;
        if (sem_init(&shm->floors[i].mutex, 1, 1) < 0){
            perror("ERROR: sem_init floor mutex");
            exit(EXIT_FAILURE);
        }
    }
}

void init_elevators(t_shm *shm, t_config *cfg)
{
    shm->delivery->current_floor = 0;
    shm->delivery->direction     = ELEV_UP;
    shm->delivery->capacity      = cfg->delivery_elevator_capacity;
    shm->delivery->current_load  = 0;
    if (sem_init(&shm->delivery->mutex,       1, 1) < 0) { perror("ERROR: sem_init deliv mutex"); exit(EXIT_FAILURE); }
    if (sem_init(&shm->delivery->request_sem, 1, 0) < 0) { perror("ERROR: sem_init deliv req");   exit(EXIT_FAILURE); }

    shm->reposition->current_floor = 0;
    shm->reposition->direction     = ELEV_UP;
    shm->reposition->capacity      = cfg->reposition_elevator_capacity;
    shm->reposition->current_load  = 0;
    if (sem_init(&shm->reposition->mutex,       1, 1) < 0) { perror("ERROR: sem_init repos mutex"); exit(EXIT_FAILURE); }
    if (sem_init(&shm->reposition->request_sem, 1, 0) < 0) { perror("ERROR: sem_init repos req");   exit(EXIT_FAILURE); }
}
