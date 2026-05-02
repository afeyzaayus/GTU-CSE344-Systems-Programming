#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "shm.h"
 
static int init_region_a(t_shm *shm, int capacity) {
    size_t size = sizeof(t_region_a) + sizeof(t_log_entry) * capacity;
    shm->a = mmap(NULL, size,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS,
                  -1, 0);
    if (shm->a == MAP_FAILED)
        return (perror("mmap region_a"), 0);
 
    memset(shm->a, 0, size);
    shm->a->capacity = capacity;
 
    for (int i = 0; i < 4; i++)
        shm->a->eof_count_per_level[i] = 0;
 
    if (!init_mutex(&shm->a->mutex))
        return (0);
    if (!init_cond(&shm->a->not_full))
        return (0);
    if (!init_cond(&shm->a->not_empty))
        return (0);
 
    return (1);
}

static int init_region_b(t_shm *shm, int capacity) {
    size_t size = sizeof(t_region_b) + sizeof(t_log_entry) * capacity;
 
    for (int i = 0; i < 4; i++) {
        shm->b[i] = mmap(NULL, size,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS,
                         -1, 0);
        if (shm->b[i] == MAP_FAILED)
            return (perror("mmap region_b"), 0);
 
        memset(shm->b[i], 0, size);
        shm->b[i]->capacity = capacity;
 
        if (!init_mutex(&shm->b[i]->mutex))
            return (0);
        if (!init_cond(&shm->b[i]->not_full))
            return (0);
        if (!init_cond(&shm->b[i]->not_empty))
            return (0);
    }
    return (1);
}

static int init_region_c(t_shm *shm) {
    size_t size = sizeof(t_region_c);
 
    shm->c = mmap(NULL, size,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS,
                  -1, 0);
    if (shm->c == MAP_FAILED)
        return (perror("mmap region_c"), 0);
 
    memset(shm->c, 0, size);
 
    if (!init_mutex(&shm->c->mutex))
        return (0);
    if (!init_cond(&shm->c->result_ready))
        return (0);
 
    for (int i = 0; i < 4; i++) {
        if (sem_init(&shm->c->sem[i], 1, 0) != 0)
            return (perror("sem_init"), 0);
    }

    for (int i = 0; i < 4; i++)
        shm->c->results[i].ready = 0;
 
    return (1);
}

static int init_region_d(t_shm *shm, int capacity) {
    size_t size = sizeof(t_region_d) + sizeof(t_log_entry) * capacity;
 
    shm->d = mmap(NULL, size,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS,
                  -1, 0);
    if (shm->d == MAP_FAILED)
        return (perror("mmap region_d"), 0);
 
    memset(shm->d, 0, size);
    shm->d->capacity = capacity;
 
    if (!init_mutex(&shm->d->mutex))
        return (0);
    if (!init_cond(&shm->d->not_full))
        return (0);
    if (!init_cond(&shm->d->not_empty))
        return (0);
 
    return (1);
}

int shm_init(t_shm *shm, t_args *args) {
    if (!init_region_a(shm, args->cap_a))
        return (fprintf(stderr, "Error: region A init failed\n"), 0);
 
    if (!init_region_b(shm, args->cap_b))
        return (fprintf(stderr, "Error: region B init failed\n"), 0);
 
    if (!init_region_c(shm))
        return (fprintf(stderr, "Error: region C init failed\n"), 0);
 
    if (!init_region_d(shm, args->cap_d))
        return (fprintf(stderr, "Error: region D init failed\n"), 0);
 
    shm->a->total_readers = args->file_count;
 
    return (1);
}