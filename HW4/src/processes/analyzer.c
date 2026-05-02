#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
#include "../../inc/analyzer.h"
#include "../../inc/shm.h"
#include "../../inc/log_entry.h"

int get_level_weight(t_level level){
    if (level == LV_ERROR)  return (4);
    if (level == LV_WARN)   return (3);
    if (level == LV_INFO)   return (2);
    if (level == LV_DEBUG)  return (1);
    return (0);
}

int count_keyword(char *text, char *word){
    int count = 0;
    int len   = strlen(word);

    if (len == 0)
        return (0);
    while (*text) {
        if (strncmp(text, word, len) == 0)
            count++;
        text++;
    }
    return (count);
}

static void add_source_hit(t_tls_data *tls, const char *source, long hits)
{
    for (int i = 0; i < tls->source_count; i++){
        if (strcmp(tls->sources[i].source, source) == 0){
            tls->sources[i].hits += hits;
            return;
        }
    }
    if (tls->source_count >= MAX_TRACKED_SOURCES)
        return;
    strncpy(tls->sources[tls->source_count].source,
            source, MAX_SOURCE_LEN - 1);
    tls->sources[tls->source_count].source[MAX_SOURCE_LEN - 1] = '\0';
    tls->sources[tls->source_count].hits = hits;
    tls->source_count++;
}

static void tls_destructor(void *ptr)
{
    t_tls_data  *tls        = (t_tls_data *)ptr;
    t_region_c  *region_c   = tls->region_c_ptr;
    t_args      *args       = tls->args_ptr;
    int          lv         = tls->level_index;

    if (!region_c || !args) {
        free(tls);
        return;
    }

    pthread_mutex_lock(&region_c->mutex);

    if (tls->thread_slot >= 0 && tls->thread_slot < MAX_WORKERS)
        region_c->results[lv].per_thread_score[tls->thread_slot] =
            tls->total_score;

    for (int i = 0; i < args->keyword_count; i++)
        region_c->results[lv].per_keyword_score[i] +=
            tls->keyword_scores[i];

    region_c->results[lv].total_entries        += tls->entry_count;
    region_c->results[lv].total_weighted_score += tls->total_score;

    for (int i = 0; i < tls->source_count; i++){
        int found = 0;
        for (int j = 0; j < 3; j++){
            if (strcmp(region_c->results[lv].top_source[j],
                       tls->sources[i].source) == 0)
            {
                region_c->results[lv].top_source_hits[j] +=
                    tls->sources[i].hits;
                found = 1;
                break;
            }
        }
        if (!found){
            int min_idx = 0;
            for (int j = 1; j < 3; j++){
                if (region_c->results[lv].top_source_hits[j] <
                    region_c->results[lv].top_source_hits[min_idx])
                    min_idx = j;
            }
            if (tls->sources[i].hits >
                region_c->results[lv].top_source_hits[min_idx])
            {
                strncpy(region_c->results[lv].top_source[min_idx],
                        tls->sources[i].source, MAX_SOURCE_LEN - 1);
                region_c->results[lv].top_source[min_idx][MAX_SOURCE_LEN - 1]
                    = '\0';
                region_c->results[lv].top_source_hits[min_idx] =
                    tls->sources[i].hits;
            }
        }
    }

    pthread_mutex_unlock(&region_c->mutex);
    free(tls);
}

static void *worker_thread_func(void *arg){
    t_worker_args   *warg   = (t_worker_args *)arg;
    t_log_entry     entry;
    int             weight  = get_level_weight((t_level)warg->level_index);

    pid_t my_tid = (pid_t)syscall(SYS_gettid);

    printf("[PID:%d][TID:%d] Worker %d started.\n",
           getpid(), (int)my_tid, warg->worker_id);

    t_tls_data *tls = calloc(1, sizeof(t_tls_data));
    if (!tls) {
        perror("calloc tls");
        return (NULL);
    }
    tls->thread_slot    = warg->worker_id;
    tls->tid            = my_tid;
    tls->level_index    = warg->level_index;
    tls->region_c_ptr   = warg->region_c;
    tls->args_ptr       = warg->args;
    pthread_setspecific(warg->tls_key, tls);

    pthread_mutex_lock(warg->lowest_tid_mutex);
    if (my_tid < *warg->lowest_tid)
        *warg->lowest_tid = my_tid;
    pthread_mutex_unlock(warg->lowest_tid_mutex);

    while (1){
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += warg->args->timeout;

        pthread_mutex_lock(&warg->region_b->mutex);

        while (warg->region_b->count == 0 && !warg->region_b->eof_posted){
            int rc = pthread_cond_timedwait(&warg->region_b->not_empty,
                                            &warg->region_b->mutex, &ts);
            if (rc != 0) {
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += warg->args->timeout;
            }
        }

        if (warg->region_b->count == 0 && warg->region_b->eof_posted){
            pthread_mutex_unlock(&warg->region_b->mutex);
            break;
        }

        region_b_pop(warg->region_b, &entry);
        pthread_cond_signal(&warg->region_b->not_full);
        pthread_mutex_unlock(&warg->region_b->mutex);

        if (entry.is_eof)
            break;

        long total_hits = 0;
        for (int i = 0; i < warg->args->keyword_count; i++){
            int    hits = count_keyword(entry.message, warg->args->keywords[i]);
            double sc   = (double)(hits * weight);
            tls->keyword_scores[i] += sc;
            tls->total_score       += sc;
            total_hits             += hits;
        }
        tls->entry_count++;

        if (total_hits > 0)
            add_source_hit(tls, entry.source, total_hits * weight);
    }

    printf("[PID:%d][TID:%d] Worker %d done. Entries: %ld, Weighted score: %.1f\n",
           getpid(), (int)my_tid, warg->worker_id,
           tls->entry_count, tls->total_score);

    pthread_barrier_wait(warg->barrier);

    return (NULL);
}

void run_analyzer(t_args *args, t_shm *shm, int level_index)
{
    const char      *level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};
    int              W             = args->worker_threads;

    printf("[PID:%d] Analyzer %s started. Workers: %d\n",
           getpid(), level_names[level_index], W);

    memset(&shm->c->results[level_index], 0, sizeof(t_level_result));
    strncpy(shm->c->results[level_index].level,
            level_names[level_index], 7);

    pthread_key_t tls_key;
    if (pthread_key_create(&tls_key, tls_destructor) != 0)
        return (void)perror("pthread_key_create");

    pthread_barrier_t barrier;
    if (pthread_barrier_init(&barrier, NULL, W) != 0){
        perror("pthread_barrier_init");
        pthread_key_delete(tls_key);
        return;
    }

    pid_t           lowest_tid       = (pid_t)0x7fffffff;
    pthread_mutex_t lowest_tid_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t       *tids  = malloc(sizeof(pthread_t) * W);
    t_worker_args   *wargs = malloc(sizeof(t_worker_args) * W);

    if (!tids || !wargs) {
        perror("malloc worker args");
        free(tids);
        free(wargs);
        pthread_barrier_destroy(&barrier);
        pthread_key_delete(tls_key);
        return;
    }

    for (int i = 0; i < W; i++) {
        wargs[i].worker_id          = i;
        wargs[i].level_index        = level_index;
        wargs[i].region_b           = shm->b[level_index];
        wargs[i].region_c           = shm->c;
        wargs[i].args               = args;
        wargs[i].tls_key            = tls_key;
        wargs[i].barrier            = &barrier;
        wargs[i].lowest_tid         = &lowest_tid;
        wargs[i].lowest_tid_mutex   = &lowest_tid_mutex;

        if (pthread_create(&tids[i], NULL, worker_thread_func, &wargs[i]) != 0) {
            perror("pthread_create worker");
            for (int j = 0; j < i; j++)
                pthread_join(tids[j], NULL);
            free(tids);
            free(wargs);
            pthread_barrier_destroy(&barrier);
            pthread_key_delete(tls_key);
            return;
        }
    }

    for (int i = 0; i < W; i++)
        pthread_join(tids[i], NULL);

    printf("[PID:%d][TID:%d] ** Reporting thread (lowest TID). Level: %s **\n",
           getpid(), (int)lowest_tid, level_names[level_index]);

    printf("[PID:%d][TID:%d] Total entries: %ld | Total weighted score: %.1f\n",
           getpid(), (int)lowest_tid,
           shm->c->results[level_index].total_entries,
           shm->c->results[level_index].total_weighted_score);

    pthread_mutex_lock(&shm->c->mutex);
    shm->c->results[level_index].ready = 1;
    pthread_cond_broadcast(&shm->c->result_ready);
    pthread_mutex_unlock(&shm->c->mutex);

    sem_post(&shm->c->sem[level_index]);

    pthread_mutex_destroy(&lowest_tid_mutex);
    pthread_barrier_destroy(&barrier);
    pthread_key_delete(tls_key);
    free(tids);
    free(wargs);

    printf("[PID:%d] Analyzer %s exiting.\n", getpid(), level_names[level_index]);
}