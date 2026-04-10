#ifndef SHM_H
#define SHM_H

#include "structs.h"

/*
 * Shared memory düzeni (tek mmap bloğu, sırayla):
 *
 *  [ t_shm_header         ]   ← sabit boyut
 *  [ t_word × word_count  ]   ← words dizisi
 *  [ t_floor × num_floors ]   ← floors dizisi
 *  [ t_elevator × 2       ]   ← delivery + reposition
 *  [ int × num_floors     ]   ← delivery elevator requests
 *  [ int × num_floors     ]   ← reposition elevator requests
 *  [ t_lc_state × total_lc]   ← letter-carrier durumları
 *
 * Tüm pointer'lar yerine offset kullan — fork sonrası
 * farklı process'lerde adres aynı kalır (MAP_SHARED + MAP_ANONYMOUS).
 */

typedef struct s_shm_header {
    int    total_words;
    int    rr_index;       /* round-robin sayacı */
    int    done_count;     /* completed word sayısı */
    int    shutdown;       /* 1 → herkes çıkıyor */
    sem_t  rr_mutex;       /* rr_index koruması */
    sem_t  done_mutex;     /* done_count koruması */
} t_shm_header;

typedef struct s_shm {
    t_shm_header  *header;
    t_word        *words;
    t_floor       *floors;
    t_elevator    *delivery;
    t_elevator    *reposition;
    int           *delivery_requests;    /* [num_floors] */
    int           *reposition_requests;  /* [num_floors] */
    t_lc_state    *lc_states;
    void          *base;   /* mmap başlangıcı (munmap için) */
    size_t         size;   /* toplam boyut */
} t_shm;

size_t  shm_calc_size(int word_count, int num_floors, int total_lc);
t_shm   shm_init(int word_count, t_line_input *words_input,
                 t_config *cfg);
void    shm_destroy(t_shm *shm, int num_floors);

#endif