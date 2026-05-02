#ifndef READER_H
# define READER_H

# include "log_entry.h"
# include "parser.h"
# include "shm.h"

typedef struct s_internal_buffer {
    t_log_entry     *data;
    int             capacity;
    int             head;
    int             tail;
    int             count;
    int             done;           
    int             active_readers; 
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;
}   t_internal_buffer;

typedef struct s_reader_thread_args{
    int                 thread_id;
    int                 reader_index; 
    char                *filepath;
    long                start_byte;
    long                end_byte;
    t_internal_buffer   *ibuf;
    int                 pipe_write_fd;
    t_args              *args;
}   t_reader_thread_args;

typedef struct s_parser_thread_args{
    t_internal_buffer   *ibuf;
    t_region_a          *region_a;
    t_args              *args;
    int                 reader_index;
}   t_parser_thread_args;

long find_line_start(FILE *fp, long pos);
void send_heartbeat(int pipe_fd, int reader_index, long line_count);
void    run_reader(t_args *args, t_shm *shm, int reader_index, int pipe_write_fd);

int ibuf_init(t_internal_buffer *ibuf, int capacity);
void ibuf_free(t_internal_buffer *ibuf);
void ibuf_push(t_internal_buffer *ibuf, t_log_entry *entry);
int ibuf_pop(t_internal_buffer *ibuf, t_log_entry *entry);
void ibuf_reader_done(t_internal_buffer *ibuf);

void *reader_thread_func(void *arg);
void *parser_thread_func(void *arg);

#endif