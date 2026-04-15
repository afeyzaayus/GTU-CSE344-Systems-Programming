#ifndef SHM_H
#define SHM_H

#include "structs.h"

typedef struct s_shm_header {
    int    total_words;
    int    rr_index;
    int    admitted_count;
    int    done_count;       
    int    shutdown;
    int    retry_count;      
    int    delivery_ops;     
    int    reposition_ops;   
    sem_t  rr_mutex;
    sem_t  done_mutex;
    sem_t  stats_mutex;      
} t_shm_header;

typedef struct s_shm {
    t_shm_header  *header;
    t_word        *words;
    t_floor       *floors;
    t_elevator    *delivery;
    t_elevator    *reposition;
    int           *delivery_requests;
    int           *reposition_requests;
    t_lc_state    *lc_states;
    void          *base;
    size_t         size;
} t_shm;

size_t  shm_calc_size(int word_count, int num_floors, int total_lc);
void    shm_assign_pointers(t_shm *shm, int word_count, int num_floors, int total_lc);
t_shm   shm_init(int word_count, t_line_input *words_input, t_config *cfg);
void    shm_destroy(t_shm *shm, int num_floors);

void    init_shm_header_values(t_shm_header *header, int word_count);
void    init_word_info(t_shm *shm, int word_count, t_line_input *words_input);
void    init_floors(t_shm *shm, int num_floors);
void    init_elevators(t_shm *shm, t_config *cfg);

#endif