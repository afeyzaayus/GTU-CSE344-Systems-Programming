#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "shm.h"

int init_mutex(pthread_mutex_t *m) {
    pthread_mutexattr_t attr;
 
    if (pthread_mutexattr_init(&attr) != 0)
        return (0);
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
        return (pthread_mutexattr_destroy(&attr), 0);
    if (pthread_mutex_init(m, &attr) != 0)
        return (pthread_mutexattr_destroy(&attr), 0);
    pthread_mutexattr_destroy(&attr);
    return (1);
}

int init_cond(pthread_cond_t *c){
    pthread_condattr_t attr;
 
    if (pthread_condattr_init(&attr) != 0)
        return (0);
    if (pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
        return (pthread_condattr_destroy(&attr), 0);
    if (pthread_cond_init(c, &attr) != 0)
        return (pthread_condattr_destroy(&attr), 0);
    pthread_condattr_destroy(&attr);
    return (1);
}

static void destroy_a(t_shm *shm, t_args *args){
    if (shm->a && shm->a != MAP_FAILED) {
        pthread_mutex_destroy(&shm->a->mutex);
        pthread_cond_destroy(&shm->a->not_full);
        pthread_cond_destroy(&shm->a->not_empty);
        size_t size = sizeof(t_region_a) + sizeof(t_log_entry) * args->cap_a;
        munmap(shm->a, size);
        shm->a = NULL;
    }
}

static void destroy_b(t_shm *shm, t_args *args){
    for (int i = 0; i < 4; i++) {
        if (shm->b[i] && shm->b[i] != MAP_FAILED) {
            pthread_mutex_destroy(&shm->b[i]->mutex);
            pthread_cond_destroy(&shm->b[i]->not_full);
            pthread_cond_destroy(&shm->b[i]->not_empty);
            size_t size = sizeof(t_region_b) + sizeof(t_log_entry) * args->cap_b;
            munmap(shm->b[i], size);
            shm->b[i] = NULL;
        }
    }
}

static void destroy_c(t_shm *shm, t_args *args){
    if (shm->c && shm->c != MAP_FAILED) {
        pthread_mutex_destroy(&shm->c->mutex);
        pthread_cond_destroy(&shm->c->result_ready);
        for (int i = 0; i < 4; i++)
            sem_destroy(&shm->c->sem[i]);
        munmap(shm->c, sizeof(t_region_c));
        shm->c = NULL;
    }
}

static void destroy_d(t_shm *shm, t_args *args){
    if (shm->d && shm->d != MAP_FAILED) {
        pthread_mutex_destroy(&shm->d->mutex);
        pthread_cond_destroy(&shm->d->not_full);
        pthread_cond_destroy(&shm->d->not_empty);
        size_t size = sizeof(t_region_d) + sizeof(t_log_entry) * args->cap_d;
        munmap(shm->d, size);
        shm->d = NULL;
    }
}

void shm_destroy(t_shm *shm, t_args *args) {
    destroy_a(shm, args);
    destroy_b(shm, args);
    destroy_c(shm, args);
    destroy_d(shm, args);
}