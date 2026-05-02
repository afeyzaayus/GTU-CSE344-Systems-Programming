#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "reader.h"
#include "log_entry.h"
#include "shm.h"

long find_line_start(FILE *fp, long pos) {
    if (pos == 0)
        return (0);

    fseek(fp, pos - 1, SEEK_SET);
    int c = fgetc(fp);

    if (c == '\n' || c == EOF)
        return (pos);

    while ((c = fgetc(fp)) != EOF && c != '\n')
        ;

    return (ftell(fp));
}

void send_heartbeat(int pipe_fd, int reader_index, long line_count){
    char buf[128];
    int  len;

    len = snprintf(buf, sizeof(buf),
                   "[R%d] %ld lines processed\n",
                   reader_index, line_count);
    if (len > 0)
        write(pipe_fd, buf, len);
}

void run_reader(t_args *args, t_shm *shm, int reader_index, int pipe_write_fd) {
    t_internal_buffer       ibuf;
    pthread_t               *reader_tids;
    pthread_t               parser_tid;
    t_reader_thread_args    *rargs;
    t_parser_thread_args    parg;
    int                     T = args->reader_threads;

    printf("[PID:%d] Reader %d started. File: %s, Threads: %d\n",
           getpid(), reader_index, args->log_files[reader_index], T);

    if (!ibuf_init(&ibuf, args->cap_b)){
        fprintf(stderr, "Reader %d: ibuf_init failed\n", reader_index);
        return;
    }

    ibuf.active_readers = T;

    FILE *fp = fopen(args->log_files[reader_index], "r");
    if (!fp) {
        perror("run_reader fopen");
        ibuf_free(&ibuf);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);

    if (file_size == 0){
        fprintf(stderr, "Reader %d: empty file, skipping\n", reader_index);
        ibuf.done = 1;
    }

    reader_tids = malloc(sizeof(pthread_t) * T);
    rargs       = malloc(sizeof(t_reader_thread_args) * T);

    if (!reader_tids || !rargs){
        perror("malloc reader args");
        ibuf_free(&ibuf);
        free(reader_tids);
        free(rargs);
        return;
    }

    long chunk = (file_size + T - 1) / T;

    for (int i = 0; i < T; i++){
        rargs[i].thread_id      = i;
        rargs[i].reader_index   = reader_index;
        rargs[i].filepath       = args->log_files[reader_index];
        rargs[i].start_byte     = i * chunk;
        rargs[i].end_byte       = (i + 1) * chunk;
        rargs[i].ibuf           = &ibuf;
        rargs[i].pipe_write_fd  = pipe_write_fd;
        rargs[i].args           = args;

        if (rargs[i].end_byte > file_size || i == T - 1)
            rargs[i].end_byte = file_size;

        printf("[PID:%d][TID:%ld] Reader thread %d: range [%ld, %ld) bytes\n",
               getpid(), (long)pthread_self(),
               i, rargs[i].start_byte, rargs[i].end_byte);
    }

    parg.ibuf           = &ibuf;
    parg.region_a       = shm->a;
    parg.args           = args;
    parg.reader_index   = reader_index;

    if (pthread_create(&parser_tid, NULL, parser_thread_func, &parg) != 0) {
        perror("pthread_create parser");
        ibuf_free(&ibuf);
        free(reader_tids);
        free(rargs);
        return;
    }

    for (int i = 0; i < T; i++){
        if (pthread_create(&reader_tids[i], NULL, reader_thread_func, &rargs[i]) != 0){
            perror("pthread_create reader thread");
            for (int j = 0; j < i; j++)
                pthread_join(reader_tids[j], NULL);
            pthread_mutex_lock(&ibuf.mutex);
            ibuf.done = 1;
            pthread_cond_signal(&ibuf.not_empty);
            pthread_mutex_unlock(&ibuf.mutex);
            pthread_join(parser_tid, NULL);
            ibuf_free(&ibuf);
            free(reader_tids);
            free(rargs);
            return;
        }
    }

    for (int i = 0; i < T; i++)
        pthread_join(reader_tids[i], NULL);

    pthread_join(parser_tid, NULL);
    ibuf_free(&ibuf);
    free(reader_tids);
    free(rargs);
    printf("[PID:%d] Reader %d exiting.\n", getpid(), reader_index);
}