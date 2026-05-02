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

void *reader_thread_func(void *arg){
    t_reader_thread_args    *rarg = (t_reader_thread_args *)arg;
    FILE                    *fp;
    char                    line[1024];
    t_log_entry             entry;
    long                    lines_read = 0;
    long                    malformed  = 0;

    printf("[PID:%d][TID:%ld] Reader thread %d: range [%ld, %ld) bytes\n",
           getpid(), (long)pthread_self(),
           rarg->thread_id, rarg->start_byte, rarg->end_byte);

    fp = fopen(rarg->filepath, "r");
    if (!fp) {
        perror("reader thread fopen");
        ibuf_reader_done(rarg->ibuf);
        return (NULL);
    }

    long real_start = find_line_start(fp, rarg->start_byte);
    fseek(fp, real_start, SEEK_SET);

    while (ftell(fp) < rarg->end_byte) {
        if (!fgets(line, sizeof(line), fp))
            break;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0')
            continue;

        if (parse_log_line(line, &entry)){
            ibuf_push(rarg->ibuf, &entry);
            lines_read++;

            if (lines_read % 50 == 0)
                send_heartbeat(rarg->pipe_write_fd,
                               rarg->reader_index, lines_read);
        }
        else
            malformed++;
    }
    fclose(fp);
    printf("[PID:%d][TID:%ld] Reader thread %d: finished, lines_read=%ld, malformed=%ld\n",
           getpid(), (long)pthread_self(),
           rarg->thread_id, lines_read, malformed);
    if (lines_read % 50 != 0)
        send_heartbeat(rarg->pipe_write_fd, rarg->reader_index, lines_read);
    ibuf_reader_done(rarg->ibuf);
    return (NULL);
}

void *parser_thread_func(void *arg) {
    t_parser_thread_args    *parg = (t_parser_thread_args *)arg;
    t_log_entry             entry;
    long                    dispatched[4] = {0, 0, 0, 0};

    while (ibuf_pop(parg->ibuf, &entry)) {
        if (entry.level < 0 || entry.level >= LV_INVALID)
            continue;

        pthread_mutex_lock(&parg->region_a->mutex);

        while (parg->region_a->count == parg->region_a->capacity)
            pthread_cond_wait(&parg->region_a->not_full, &parg->region_a->mutex);

        region_a_push(parg->region_a, &entry);
        dispatched[entry.level]++;

        pthread_cond_signal(&parg->region_a->not_empty);
        pthread_mutex_unlock(&parg->region_a->mutex);
    }

    t_log_entry eof_entry;
    memset(&eof_entry, 0, sizeof(t_log_entry));
    eof_entry.is_eof = 1;

    for (int lv = 0; lv < 4; lv++) {
        eof_entry.level = (t_level)lv;

        pthread_mutex_lock(&parg->region_a->mutex);

        while (parg->region_a->count == parg->region_a->capacity)
            pthread_cond_wait(&parg->region_a->not_full, &parg->region_a->mutex);

        region_a_push(parg->region_a, &eof_entry);
        parg->region_a->eof_count_per_level[lv]++;

        pthread_cond_signal(&parg->region_a->not_empty);
        pthread_mutex_unlock(&parg->region_a->mutex);
    }

    printf("[PID:%d] Parser thread: dispatched E:%ld W:%ld I:%ld D:%ld -> Region A\n",
           getpid(),
           dispatched[LV_ERROR],
           dispatched[LV_WARN],
           dispatched[LV_INFO],
           dispatched[LV_DEBUG]);

    return (NULL);
}