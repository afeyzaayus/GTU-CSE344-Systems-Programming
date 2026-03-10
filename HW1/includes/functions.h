#ifndef FUNCTIONS_H
# define FUNCTIONS_H

#include "struct.h"


int parse_arguments(int argc, char **argv, t_file *criteria);
int regex_match(t_program *program, const char *filename);
void search_directory(const char *curr_path, t_program *program, int depth);


#endif
