#ifndef ANALYZER_H
# define ANALYZER_H

# include "log_entry.h"
# include "parser.h"
# include "shm.h"

#define MAX_TRACKED_SOURCES 256

typedef struct s_source_entry{
    char    source[MAX_SOURCE_LEN];
    long    hits;
}   t_source_entry;

typedef struct s_tls_data{
    double          keyword_scores[MAX_KEYWORDS];
    long            entry_count;
    double          total_score;
    t_source_entry  sources[MAX_TRACKED_SOURCES];
    int             source_count;
    int             thread_slot;
    pid_t           tid;
    int             level_index;
    t_region_c      *region_c_ptr;
    t_args          *args_ptr;
}   t_tls_data;

typedef struct s_worker_args{
    int                 worker_id;
    int                 level_index;
    t_region_b          *region_b;
    t_region_c          *region_c;
    t_args              *args;
    pthread_key_t       tls_key;
    pthread_barrier_t   *barrier;
    pid_t               *lowest_tid;
    pthread_mutex_t     *lowest_tid_mutex;
}   t_worker_args;

void    run_analyzer(t_args *args, t_shm *shm, int level_index);

int     get_level_weight(t_level level);
int     count_keyword(char *text, char *word);

#endif