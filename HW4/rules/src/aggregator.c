#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include "aggregator.h"
#include "shm.h"
#include "log_entry.h"

static double calc_high_priority_score(t_shm *shm, t_args *args){
    t_log_entry entry;
    double      score = 0.0;

    pthread_mutex_lock(&shm->d->mutex);

    while (!shm->d->dispatcher_done){
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += args->timeout;
        int rc = pthread_cond_timedwait(&shm->d->not_empty, &shm->d->mutex, &ts);
        if (rc != 0)
            break; 
    }

    while (shm->d->count > 0) {
        region_d_pop(shm->d, &entry);
        pthread_mutex_unlock(&shm->d->mutex);

        if (!entry.is_eof) {
            int weight = 0;
            if (entry.level == LV_ERROR)       weight = 4;
            else if (entry.level == LV_WARN)   weight = 3;
            else if (entry.level == LV_INFO)   weight = 2;
            else if (entry.level == LV_DEBUG)  weight = 1;

            for (int i = 0; i < args->keyword_count; i++) {
                char *text = entry.message;
                char *word = args->keywords[i];
                int   len  = strlen(word);
                int   cnt  = 0;
                while (*text) {
                    if (strncmp(text, word, len) == 0)
                        cnt++;
                    text++;
                }
                score += (double)(cnt * weight);
            }
        }
        pthread_mutex_lock(&shm->d->mutex);
    }
    pthread_mutex_unlock(&shm->d->mutex);
    return (score);
}

static void sort_results(t_level_result *results, int *order){
    for (int i = 0; i < 4; i++)
        order[i] = i;

    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (results[order[j]].total_weighted_score >
                results[order[i]].total_weighted_score)
            {
                int tmp  = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
}

static int write_txt_output(t_args *args, t_shm *shm,
                             int *order, double hp_score)
{
    FILE *fp = fopen(args->output_txt, "w");
    if (!fp)
        return (perror("fopen output_txt"), 0);

    fprintf(fp, "KEYWORD_LIST: ");
    for (int i = 0; i < args->keyword_count; i++) {
        fprintf(fp, "%s", args->keywords[i]);
        if (i < args->keyword_count - 1)
            fprintf(fp, ",");
    }
    fprintf(fp, "\n");

    fprintf(fp, "FILES: %d\n", args->file_count);

    double total = 0.0;
    for (int i = 0; i < 4; i++)
        total += shm->c->results[i].total_weighted_score;
    fprintf(fp, "TOTAL_WEIGHTED_SCORE: %.1f\n", total);

    fprintf(fp, "HIGH_PRIORITY_SCORE: %.1f\n", hp_score);

    fprintf(fp, "# Levels sorted by total_weighted_score DESC\n");
    fprintf(fp, "%-5s  %7s  %14s", "LEVEL", "ENTRIES", "WEIGHTED_SCORE");
    for (int i = 0; i < args->keyword_count; i++)
        fprintf(fp, "  %10s", args->keywords[i]);
    fprintf(fp, "\n");

    for (int si = 0; si < 4; si++) {
        int i = order[si];
        t_level_result *r = &shm->c->results[i];
        fprintf(fp, "%-5s  %7ld  %14.1f",
                r->level, r->total_entries, r->total_weighted_score);
        for (int k = 0; k < args->keyword_count; k++)
            fprintf(fp, "  %10.1f", r->per_keyword_score[k]);
        fprintf(fp, "\n");
    }

    fprintf(fp, "# Top-3 sources per level\n");
    for (int si = 0; si < 4; si++) {
        int i = order[si];
        t_level_result *r = &shm->c->results[i];
        fprintf(fp, "%-5s", r->level);
        for (int j = 0; j < 3; j++)
        {
            if (r->top_source[j][0] != '\0')
                fprintf(fp, " %s:%ld", r->top_source[j], r->top_source_hits[j]);
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "# Per-thread contributions (weighted score)\n");
    for (int si = 0; si < 4; si++)
    {
        int i = order[si];
        t_level_result *r = &shm->c->results[i];
        fprintf(fp, "%-5s", r->level);
        for (int w = 0; w < args->worker_threads; w++)
            fprintf(fp, " thread_%d:%.1f", w, r->per_thread_score[w]);
        fprintf(fp, "\n");
    }

    fclose(fp);
    return (1);
}

static int write_bin_output(t_args *args, t_shm *shm,
                             double total_weighted, double hp_score)
{
    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", args->output_bin);

    FILE *fp = fopen(tmp_path, "wb");
    if (!fp)
        return (perror("fopen binary tmp"), 0);

    t_bin_header hdr;
    hdr.magic                  = BINARY_MAGIC;
    hdr.version                = BINARY_VERSION;
    hdr.num_levels             = 4;
    hdr.num_keywords           = (unsigned int)args->keyword_count;
    hdr.total_weighted         = total_weighted;
    hdr.high_priority_weighted = hp_score;

    if (fwrite(&hdr, sizeof(t_bin_header), 1, fp) != 1){
        fprintf(stderr, "Error: binary header fwrite failed\n");
        fclose(fp);
        return (0);
    }

    for (int i = 0; i < 4; i++) {
        if (fwrite(&shm->c->results[i], sizeof(t_level_result), 1, fp) != 1){
            fprintf(stderr, "Error: binary level %d fwrite failed\n", i);
            fclose(fp);
            return (0);
        }
    }
    fclose(fp);
    if (rename(tmp_path, args->output_bin) != 0)
        return (perror("rename binary"), 0);

    return (1);
}

void run_aggregator(t_args *args, t_shm *shm)
{
    printf("[PID:%d] Aggregator started. Waiting for 4 levels...\n", getpid());

    for (int i = 0; i < 4; i++) {
        const char *level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += args->timeout * 4; 

        int rc = sem_timedwait(&shm->c->sem[i], &ts);
        if (rc != 0) {
            fprintf(stderr,
                    "[Aggregator] Timeout waiting for level %d result\n", i);
        }
        else {
            printf("[PID:%d] %s result received.\n",
                   getpid(), level_names[i]);
        }
    }

    printf("[PID:%d] All results received. Writing output files...\n", getpid());

    int order[4];
    sort_results(shm->c->results, order);
    double hp_score = calc_high_priority_score(shm, args);
    shm->c->high_priority_score = hp_score;
    double total_weighted = 0.0;
    for (int i = 0; i < 4; i++)
        total_weighted += shm->c->results[i].total_weighted_score;

    if (!write_txt_output(args, shm, order, hp_score))
        fprintf(stderr, "[Aggregator] Warning: txt output failed\n");

    if (!write_bin_output(args, shm, total_weighted, hp_score))
        fprintf(stderr, "[Aggregator] Warning: binary output failed\n");

    printf("[PID:%d] Output files written: %s, %s\n",
           getpid(), args->output_txt, args->output_bin);

    printf("[PID:%d] Aggregator exiting.\n", getpid());
}