#include "../../inc/shm.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

size_t shm_calc_size(int word_count, int num_floors, int total_lc)
{
    size_t size = 0;

    size += sizeof(t_shm_header);
    size += sizeof(t_word)      * word_count;
    size += sizeof(t_floor)     * num_floors;
    size += sizeof(t_elevator)  * 2;
    size += sizeof(int)         * num_floors * 2;
    size += sizeof(t_lc_state)  * total_lc;

    return size;
}

static void *bump(void *base, size_t *offset, size_t bytes)
{
    void *ptr = (char *)base + *offset;
    *offset += bytes;
    return ptr;
}

void shm_assign_pointers(t_shm *shm, int word_count, int num_floors, int total_lc){
    size_t offset = 0;

    shm->header               = bump(shm->base, &offset, sizeof(t_shm_header));
    shm->words                = bump(shm->base, &offset, sizeof(t_word) * word_count);
    shm->floors               = bump(shm->base, &offset, sizeof(t_floor) * num_floors);
    shm->delivery             = bump(shm->base, &offset, sizeof(t_elevator));
    shm->reposition           = bump(shm->base, &offset, sizeof(t_elevator));
    shm->delivery_requests    = bump(shm->base, &offset, sizeof(int) * num_floors);
    shm->reposition_requests  = bump(shm->base, &offset, sizeof(int) * num_floors);
    shm->lc_states            = bump(shm->base, &offset, sizeof(t_lc_state) * total_lc);
}

t_shm shm_init(int word_count, t_line_input *words_input, t_config *cfg)
{
    t_shm  shm;
    int    total_lc;

    total_lc = cfg->num_floors * cfg->letter_carriers_per_floor;
    shm.size = shm_calc_size(word_count, cfg->num_floors, total_lc);

    shm.base = mmap(NULL, shm.size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS,
                    -1, 0);
    if (shm.base == MAP_FAILED) {
        perror("ERROR: mmap");
        exit(EXIT_FAILURE);
    }
    memset(shm.base, 0, shm.size);

    shm_assign_pointers(&shm, word_count, cfg->num_floors, total_lc);
    init_shm_header_values(shm.header, word_count);
    init_word_info(&shm, word_count, words_input);
    init_floors(&shm, cfg->num_floors);
    init_elevators(&shm, cfg);

    printf("Shared memory initialized (%zu bytes)\n", shm.size);
    printf("Synchronization primitives created\n");

    return shm;
}

void shm_destroy(t_shm *shm, int num_floors)
{
    int i;

    sem_destroy(&shm->header->rr_mutex);
    sem_destroy(&shm->header->done_mutex);
    sem_destroy(&shm->header->stats_mutex);

    for (i = 0; i < shm->header->total_words; i++)
        sem_destroy(&shm->words[i].word_mutex);

    for (i = 0; i < num_floors; i++)
        sem_destroy(&shm->floors[i].mutex);

    sem_destroy(&shm->delivery->mutex);
    sem_destroy(&shm->delivery->request_sem);
    sem_destroy(&shm->reposition->mutex);
    sem_destroy(&shm->reposition->request_sem);

    if (munmap(shm->base, shm->size) < 0)
        perror("munmap");
}