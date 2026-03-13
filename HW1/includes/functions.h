#ifndef FUNCTIONS_H
# define FUNCTIONS_H

#include "struct.h"
#include <signal.h>
#include <sys/stat.h>

extern volatile sig_atomic_t g_stop_requested;

int parse_arguments(int argc, char **argv, t_file *criteria);
int regex_match(t_file *criteria, const char *filename);
void search_directory(const char *curr_path, t_program *program, int depth);
int compare(struct stat *st, t_program *program, const char *file_name);
void print_path(t_path_info *p);
void print_tree(const char *name, int depth, int is_dir);
void handle_sigint(int sig);
int setup_signal_handlers(void);
void check_signal(void);

#endif
