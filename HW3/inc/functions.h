#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "structs.h"
#include "shm.h"

/* file_control.c */
int          open_file(const char *filename);
void         trim_newline(char *line);
int          is_word_valid(char *word);
int          is_valid_line(char *line);
t_line_input parse_line(char *line);
t_line_input *ft_read_input(const char *filename, int *count);

/* args.c */
void         print_usage(const char *prog);
void         parse_args(int argc, char **argv, t_config *cfg);
void         validate_args(t_config *cfg);

/* shm.c */
size_t       shm_calc_size(int word_count, int num_floors, int total_lc);
t_shm        shm_init(int word_count, t_line_input *words_input, t_config *cfg);
void         shm_destroy(t_shm *shm, int num_floors);

#endif