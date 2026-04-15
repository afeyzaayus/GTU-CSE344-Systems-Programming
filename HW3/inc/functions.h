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
t_line_input *read_input(const char *filename, int *count);
void         validate_words(t_line_input *words, int word_count, t_config *cfg);

/* args.c */
void         print_usage(const char *prog);
void         parse_args(int argc, char **argv, t_config *cfg);
void         validate_args(t_config *cfg);

#endif