#include "../inc/shm.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Boyut hesabı                                                         */
/* ------------------------------------------------------------------ */

size_t shm_calc_size(int word_count, int num_floors, int total_lc)
{
    size_t size = 0;

    size += sizeof(t_shm_header);
    size += sizeof(t_word)      * word_count;
    size += sizeof(t_floor)     * num_floors;
    size += sizeof(t_elevator)  * 2;
    size += sizeof(int)         * num_floors * 2; /* delivery + reposition requests */
    size += sizeof(t_lc_state)  * total_lc;

    return size;
}

/* ------------------------------------------------------------------ */
/* Yardımcı: offset ile pointer hesapla, offset ilerlet                */
/* ------------------------------------------------------------------ */

static void *bump(void *base, size_t *offset, size_t bytes)
{
    void *ptr = (char *)base + *offset;
    *offset += bytes;
    return ptr;
}

/* ------------------------------------------------------------------ */
/* Shared memory başlatma                                               */
/* ------------------------------------------------------------------ */

t_shm shm_init(int word_count, t_line_input *words_input, t_config *cfg)
{
    t_shm  shm;
    size_t offset = 0;
    int    total_lc;
    int    i;
    int    j;

    total_lc = cfg->num_floors * cfg->letter_carriers_per_floor;
    shm.size = shm_calc_size(word_count, cfg->num_floors, total_lc);

    /* tek anonim mmap bloğu — tüm fork'lar aynı sayfaları görür */
    shm.base = mmap(NULL, shm.size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS,
                    -1, 0);
    if (shm.base == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    memset(shm.base, 0, shm.size);

    /* pointer'ları blok içine yerleştir */
    shm.header               = bump(shm.base, &offset, sizeof(t_shm_header));
    shm.words                = bump(shm.base, &offset, sizeof(t_word)     * word_count);
    shm.floors               = bump(shm.base, &offset, sizeof(t_floor)    * cfg->num_floors);
    shm.delivery             = bump(shm.base, &offset, sizeof(t_elevator));
    shm.reposition           = bump(shm.base, &offset, sizeof(t_elevator));
    shm.delivery_requests    = bump(shm.base, &offset, sizeof(int) * cfg->num_floors);
    shm.reposition_requests  = bump(shm.base, &offset, sizeof(int) * cfg->num_floors);
    shm.lc_states            = bump(shm.base, &offset, sizeof(t_lc_state) * total_lc);

    /* ---------------------------------------------------------------- */
    /* Header                                                            */
    /* ---------------------------------------------------------------- */
    shm.header->total_words = word_count;
    shm.header->rr_index    = 0;
    shm.header->done_count  = 0;
    shm.header->shutdown    = 0;

    if (sem_init(&shm.header->rr_mutex,   1, 1) < 0) { perror("sem_init rr");   exit(EXIT_FAILURE); }
    if (sem_init(&shm.header->done_mutex, 1, 1) < 0) { perror("sem_init done"); exit(EXIT_FAILURE); }

    /* ---------------------------------------------------------------- */
    /* Words                                                             */
    /* ---------------------------------------------------------------- */
    for (i = 0; i < word_count; i++)
    {
        t_word *w = &shm.words[i];

        w->word_id      = words_input[i].word_id;
        w->sorting_floor = words_input[i].sorting_floor;
        w->arrival_floor = -1;  /* WC atandığında doldurulur */
        w->word_len     = (int)strlen(words_input[i].word);

        strncpy(w->original, words_input[i].word, MAX_WORD_LEN - 1);
        w->original[MAX_WORD_LEN - 1] = '\0';

        /* sorting_area, occupied, fixed, claimed, admitted, completed → 0 (memset) */

        /* karakter görevlerini oluştur */
        for (j = 0; j < w->word_len; j++)
        {
            w->tasks[j].word_id        = w->word_id;
            w->tasks[j].character      = w->original[j];
            w->tasks[j].original_index = j;
            w->tasks[j].src_floor      = -1;  /* admission sırasında atanır */
            w->tasks[j].dest_floor     = w->sorting_floor;
            w->tasks[j].claimed        = 0;
            w->tasks[j].delivered      = 0;
        }

        if (sem_init(&w->word_mutex, 1, 1) < 0)
        {
            perror("sem_init word_mutex");
            exit(EXIT_FAILURE);
        }
    }

    /* ---------------------------------------------------------------- */
    /* Floors                                                            */
    /* ---------------------------------------------------------------- */
    for (i = 0; i < cfg->num_floors; i++)
    {
        shm.floors[i].current_word_count = 0;
        if (sem_init(&shm.floors[i].mutex, 1, 1) < 0)
        {
            perror("sem_init floor mutex");
            exit(EXIT_FAILURE);
        }
    }

    /* ---------------------------------------------------------------- */
    /* Elevators                                                         */
    /* ---------------------------------------------------------------- */
    shm.delivery->current_floor = 0;
    shm.delivery->direction     = ELEV_UP;
    shm.delivery->capacity      = cfg->delivery_elevator_capacity;
    shm.delivery->current_load  = 0;
    if (sem_init(&shm.delivery->mutex,       1, 1) < 0) { perror("sem_init deliv mutex"); exit(EXIT_FAILURE); }
    if (sem_init(&shm.delivery->request_sem, 1, 0) < 0) { perror("sem_init deliv req");   exit(EXIT_FAILURE); }

    shm.reposition->current_floor = 0;
    shm.reposition->direction     = ELEV_UP;
    shm.reposition->capacity      = cfg->reposition_elevator_capacity;
    shm.reposition->current_load  = 0;
    if (sem_init(&shm.reposition->mutex,       1, 1) < 0) { perror("sem_init repos mutex"); exit(EXIT_FAILURE); }
    if (sem_init(&shm.reposition->request_sem, 1, 0) < 0) { perror("sem_init repos req");   exit(EXIT_FAILURE); }

    /* requests dizileri memset ile 0 — zaten temizlendi */

    printf("Shared memory initialized (%zu bytes)\n", shm.size);
    printf("Synchronization primitives created\n");

    return shm;
}

/* ------------------------------------------------------------------ */
/* Temizlik                                                             */
/* ------------------------------------------------------------------ */

void shm_destroy(t_shm *shm, int num_floors)
{
    int i;

    sem_destroy(&shm->header->rr_mutex);
    sem_destroy(&shm->header->done_mutex);

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