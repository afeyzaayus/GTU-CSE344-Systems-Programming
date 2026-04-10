#ifndef STRUCTS_H
#define STRUCTS_H

#include <semaphore.h>
#include <sys/types.h>

#define MAX_WORD_LEN 64

typedef struct s_config {
    int   num_floors;
    int   word_carriers_per_floor;
    int   letter_carriers_per_floor;
    int   sorting_processes_per_floor;
    int   max_words_per_floor;
    int   delivery_elevator_capacity;
    int   reposition_elevator_capacity;
    char  *input_file;
    char  *output_file;
} t_config;

typedef struct s_line_input {
    int   word_id;
    char  *word;
    int   sorting_floor;
} t_line_input;

typedef struct s_char_task {
    int   word_id;
    char  character;
    int   original_index;
    int   src_floor;
    int   dest_floor;
    int   claimed;
    int   delivered;
} t_char_task;

typedef struct s_word {
    int          word_id;
    char         original[MAX_WORD_LEN];
    int          word_len;
    int          arrival_floor;
    int          sorting_floor;

    char         sorting_area[MAX_WORD_LEN];
    int          occupied[MAX_WORD_LEN];
    int          fixed[MAX_WORD_LEN];

    t_char_task  tasks[MAX_WORD_LEN];

    int          claimed;
    int          admitted;
    int          completed;

    sem_t        word_mutex;
} t_word;

typedef struct s_floor {
    int    current_word_count;
    sem_t  mutex;
} t_floor;

#define ELEV_UP   1
#define ELEV_DOWN (-1)

typedef struct s_elevator {
    int    current_floor;
    int    direction;
    int    capacity;
    int    current_load;
    sem_t  mutex;
    sem_t  request_sem;
    /* requests[num_floors] dizisi shm_init içinde offset ile konumlandırılır */
} t_elevator;

typedef struct s_lc_state {
    pid_t  pid;
    int    current_floor;
    int    idle;
} t_lc_state;

#endif