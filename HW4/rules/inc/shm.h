#ifndef SHM_H
# define SHM_H

# include <pthread.h>
# include <semaphore.h>
# include "log_entry.h"
# include "argument_parsing.h"

typedef struct s_level_result {
    char    level[8];                           // "ERROR", "WARN", "INFO", "DEBUG"
    long    total_entries;                      // bu level'da işlenen toplam entry
    double  total_weighted_score;               // tüm keyword'lerin toplam weighted score'u
    double  per_keyword_score[MAX_KEYWORDS];    // her keyword için ayrı weighted score
    double  per_thread_score[MAX_WORKERS];      // her worker thread'in bireysel katkısı
    char    top_source[3][MAX_SOURCE_LEN];      // en yüksek hit'e sahip 3 SOURCE alanı
    long    top_source_hits[3];                 // bunların weighted hit sayıları
    int     ready;                              // 1 = Analyzer bu level'ı tamamladı
}   t_level_result;

typedef struct s_region_a {
    pthread_mutex_t     mutex;                  // PROCESS_SHARED
    pthread_cond_t      not_full;               // PROCESS_SHARED — producer bekler
    pthread_cond_t      not_empty;              // PROCESS_SHARED — consumer bekler
    int                 eof_count_per_level[4]; // her level için kaç EOF geldi
    int                 total_readers;          // parent fork'tan önce set eder
    int                 head;
    int                 tail;
    int                 count;
    int                 capacity;
    t_log_entry         data[];                 // flexible array member
}   t_region_a;

typedef struct s_region_b{
    pthread_mutex_t     mutex;      // PROCESS_SHARED
    pthread_cond_t      not_full;   // PROCESS_SHARED
    pthread_cond_t      not_empty;  // PROCESS_SHARED
    int                 eof_posted; // Dispatcher bu level'ın EOF'unu koydu mu?
    int                 head;
    int                 tail;
    int                 count;
    int                 capacity;
    t_log_entry         data[];     // flexible array member
}   t_region_b;

typedef struct s_region_c{
    pthread_mutex_t     mutex;              // PROCESS_SHARED
    pthread_cond_t      result_ready;       // PROCESS_SHARED
    sem_t               sem[4];             // her level için bir semaphore (process-shared)
    t_level_result      results[4];         // LV_ERROR=0, LV_WARN=1, LV_INFO=2, LV_DEBUG=3
    double              high_priority_score;// Aggregator hesaplar, parent okur
}   t_region_c;

typedef struct s_region_d{
    pthread_mutex_t     mutex;          // PROCESS_SHARED
    pthread_cond_t      not_full;       // PROCESS_SHARED
    pthread_cond_t      not_empty;      // PROCESS_SHARED
    int                 dispatcher_done;// Dispatcher bitince 1 yapar
    int                 head;
    int                 tail;
    int                 count;
    int                 capacity;
    t_log_entry         data[];         // flexible array member
}   t_region_d;

typedef struct s_shm{
    t_region_a  *a;         // Region A pointer (mmap ile ayrılmış)
    t_region_b  *b[4];      // Region B pointer'ları (her level için ayrı mmap)
    t_region_c  *c;         // Region C pointer
    t_region_d  *d;         // Region D pointer
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