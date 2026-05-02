#ifndef SHM_H
# define SHM_H

# include <pthread.h>
# include <semaphore.h>
# include "log_entry.h"
# include "parser.h"

typedef struct s_level_result {
    char    level[8];                           
    long    total_entries;                      
    double  total_weighted_score;               
    double  per_keyword_score[MAX_KEYWORDS];    
    double  per_thread_score[MAX_WORKERS];      
    char    top_source[3][MAX_SOURCE_LEN];      
    long    top_source_hits[3];                 
    int     ready;                              
}   t_level_result;

typedef struct s_region_a {
    pthread_mutex_t     mutex;                  
    pthread_cond_t      not_full;               
    pthread_cond_t      not_empty;              
    int                 eof_count_per_level[4]; 
    int                 total_readers;          
    int                 head;
    int                 tail;
    int                 count;
    int                 capacity;
    t_log_entry         data[];                 
}   t_region_a;

typedef struct s_region_b{
    pthread_mutex_t     mutex;      
    pthread_cond_t      not_full;   
    pthread_cond_t      not_empty;  
    int                 eof_posted; 
    int                 head;
    int                 tail;
    int                 count;
    int                 capacity;
    t_log_entry         data[];     
}   t_region_b;

typedef struct s_region_c{
    pthread_mutex_t     mutex;              
    pthread_cond_t      result_ready;       
    sem_t               sem[4];             
    t_level_result      results[4];         
    double              high_priority_score;
}   t_region_c;

typedef struct s_region_d{
    pthread_mutex_t     mutex;          
    pthread_cond_t      not_full;       
    pthread_cond_t      not_empty;      
    int                 dispatcher_done;
    int                 head;
    int                 tail;
    int                 count;
    int                 capacity;
    t_log_entry         data[];         
}   t_region_d;

typedef struct s_shm{
    t_region_a  *a;         
    t_region_b  *b[4];      
    t_region_c  *c;         
    t_region_d  *d;         
}   t_shm;

int     shm_init(t_shm *shm, t_args *args);
void    shm_destroy(t_shm *shm, t_args *args);
int     region_a_push(t_region_a *a, t_log_entry *entry);
int     region_a_pop(t_region_a *a, t_log_entry *entry);
int     region_b_push(t_region_b *b, t_log_entry *entry);
int     region_b_pop(t_region_b *b, t_log_entry *entry);
int     region_d_push(t_region_d *d, t_log_entry *entry);
int     region_d_pop(t_region_d *d, t_log_entry *entry);
int     init_mutex(pthread_mutex_t *m);
int     init_cond(pthread_cond_t *c);

#endif