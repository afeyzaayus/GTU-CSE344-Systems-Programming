#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../../inc/reader.h"
#include "../../inc/log_entry.h"
#include "../../inc/shm.h"

int ibuf_init(t_internal_buffer *ibuf, int capacity) {
    ibuf->data = malloc(sizeof(t_log_entry) * capacity);
    if (!ibuf->data)
        return (perror("ibuf malloc"), 0);

    ibuf->capacity      = capacity;
    ibuf->head          = 0;
    ibuf->tail          = 0;
    ibuf->count         = 0;
    ibuf->done          = 0;
    ibuf->active_readers = 0;

    if (pthread_mutex_init(&ibuf->mutex, NULL) != 0)
        return (perror("ibuf mutex_init"), 0);
    if (pthread_cond_init(&ibuf->not_full, NULL) != 0)
        return (perror("ibuf cond not_full"), 0);
    if (pthread_cond_init(&ibuf->not_empty, NULL) != 0)
        return (perror("ibuf cond not_empty"), 0);

    return (1);
}

void ibuf_free(t_internal_buffer *ibuf){
    pthread_mutex_destroy(&ibuf->mutex);
    pthread_cond_destroy(&ibuf->not_full);
    pthread_cond_destroy(&ibuf->not_empty);
    free(ibuf->data);
}

void ibuf_push(t_internal_buffer *ibuf, t_log_entry *entry){
    pthread_mutex_lock(&ibuf->mutex);

    while (ibuf->count == ibuf->capacity)
        pthread_cond_wait(&ibuf->not_full, &ibuf->mutex);

    ibuf->data[ibuf->tail] = *entry;
    ibuf->tail = (ibuf->tail + 1) % ibuf->capacity;
    ibuf->count++;

    pthread_cond_signal(&ibuf->not_empty);
    pthread_mutex_unlock(&ibuf->mutex);
}

int ibuf_pop(t_internal_buffer *ibuf, t_log_entry *entry){
    pthread_mutex_lock(&ibuf->mutex);

    while (ibuf->count == 0 && !ibuf->done)
        pthread_cond_wait(&ibuf->not_empty, &ibuf->mutex);

    if (ibuf->count == 0 && ibuf->done){
        pthread_mutex_unlock(&ibuf->mutex);
        return (0); 
    }

    *entry = ibuf->data[ibuf->head];
    ibuf->head = (ibuf->head + 1) % ibuf->capacity;
    ibuf->count--;

    pthread_cond_signal(&ibuf->not_full);
    pthread_mutex_unlock(&ibuf->mutex);
    return (1);
}

void ibuf_reader_done(t_internal_buffer *ibuf){
    pthread_mutex_lock(&ibuf->mutex);
    ibuf->active_readers--;
    if (ibuf->active_readers == 0)
    {
        ibuf->done = 1;
        pthread_cond_signal(&ibuf->not_empty);
    }
    pthread_mutex_unlock(&ibuf->mutex);
}