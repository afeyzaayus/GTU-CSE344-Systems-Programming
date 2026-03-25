#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "structs.h"

void parse_arguments(int argc, char **argv, t_args *args);
int regex(t_args *args, const char *filename);
void check_directory(const char *path);
int search_directory(const char *path, t_args *args);
int root_subdirs(const char *root, char subdirs[][1024]);

#endif