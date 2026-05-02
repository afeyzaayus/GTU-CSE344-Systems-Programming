#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "../../inc/dispatcher.h"
#include "../../inc/shm.h"
#include "../../inc/log_entry.h"

static int is_priority_source(t_args *args, const char *source){
    for (int i = 0; i < args->priority_count; i++) {
        if (strcmp(args->priority_sources[i], source) == 0)
            return (1);
    }
    return (0);
}

static void send_eof_to_region_b(t_region_b *b){
    t_log_entry eof_entry;
    memset(&eof_entry, 0, sizeof(t_log_entry));
    eof_entry.is_eof = 1;

    pthread_mutex_lock(&b->mutex);
    while (b->count == b->capacity)
        pthread_cond_wait(&b->not_full, &b->mutex);
    region_b_push(b, &eof_entry);
    b->eof_posted = 1;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}

static void push_to_region_d(t_region_d *d, t_log_entry *entry, int timeout_sec){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_sec;

    pthread_mutex_lock(&d->mutex);
    while (d->count == d->capacity){
        int rc = pthread_cond_timedwait(&d->not_full, &d->mutex, &ts);
        if (rc != 0){
            fprintf(stderr, "[Dispatcher] Region D full timeout, dropping entry\n");
            pthread_mutex_unlock(&d->mutex);
            return;
        }
    }
    region_d_push(d, entry);
    pthread_cond_signal(&d->not_empty);
    pthread_mutex_unlock(&d->mutex);
}

static void push_to_region_b(t_region_b *b, t_log_entry *entry, int timeout_sec){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_sec;

    pthread_mutex_lock(&b->mutex);
    while (b->count == b->capacity){
        int rc = pthread_cond_timedwait(&b->not_full, &b->mutex, &ts);
        if (rc != 0){
            fprintf(stderr, "[Dispatcher] Region B full timeout, dropping entry\n");
            pthread_mutex_unlock(&b->mutex);
            return;
        }
    }
    region_b_push(b, entry);
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}

void run_dispatcher(t_args *args, t_shm *shm)
{
    t_log_entry     entry;
    int             eof_forwarded[4]  = {0, 0, 0, 0};
    int             finished_levels   = 0;
    int             total_readers     = shm->a->total_readers;
    int             timeout_sec       = args->timeout;

    printf("[PID:%d] Dispatcher started.\n", getpid());

    while (finished_levels < 4){
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_sec;

        pthread_mutex_lock(&shm->a->mutex);

        while (shm->a->count == 0) {
            int rc = pthread_cond_timedwait(&shm->a->not_empty,
                                            &shm->a->mutex, &ts);
            if (rc != 0) {
                int all_done = 1;
                for (int lv = 0; lv < 4; lv++){
                    if (!eof_forwarded[lv]){
                        all_done = 0;
                        break;
                    }
                }
                if (all_done){
                    pthread_mutex_unlock(&shm->a->mutex);
                    goto dispatcher_done;
                }
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += timeout_sec;
            }
        }

        region_a_pop(shm->a, &entry);
        pthread_cond_signal(&shm->a->not_full);
        pthread_mutex_unlock(&shm->a->mutex);

        if (entry.is_eof){
            int lv = (int)entry.level;
            if (lv < 0 || lv >= 4)
                continue;

            if (!eof_forwarded[lv]
                && shm->a->eof_count_per_level[lv] >= total_readers){
                send_eof_to_region_b(shm->b[lv]);
                eof_forwarded[lv] = 1;
                finished_levels++;

                printf("[PID:%d] EOF forwarded to Region B level %d. (%d/4 done)\n",
                       getpid(), lv, finished_levels);
            }
            continue;
        }

        int lv = (int)entry.level;
        if (lv < 0 || lv >= 4)
            continue;

        int priority = is_priority_source(args, entry.source);

        printf("[PID:%d] Routed entry to %s buffer. High-priority: %s (source: %s)\n",
               getpid(),
               lv == 0 ? "ERROR" : lv == 1 ? "WARN" : lv == 2 ? "INFO" : "DEBUG",
               priority ? "YES" : "NO",
               entry.source);

        if (priority)
            push_to_region_d(shm->d, &entry, timeout_sec);

        push_to_region_b(shm->b[lv], &entry, timeout_sec);
    }

dispatcher_done:
    for (int lv = 0; lv < 4; lv++){
        if (!eof_forwarded[lv]) {
            send_eof_to_region_b(shm->b[lv]);
            fprintf(stderr, "[Dispatcher] Forced EOF to level %d (timeout)\n", lv);
        }
    }

    pthread_mutex_lock(&shm->d->mutex);
    shm->d->dispatcher_done = 1;
    pthread_cond_signal(&shm->d->not_empty);
    pthread_mutex_unlock(&shm->d->mutex);

    printf("[PID:%d] All EOF markers forwarded to Region B. Exiting.\n", getpid());
}