#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "structs.h"
#include <sys/types.h>
#include <signal.h>


extern pid_t g_worker_pids[8];
extern int   g_num_workers;
extern int          g_pipes[8][2];

extern volatile sig_atomic_t g_partial_matches;
extern volatile sig_atomic_t g_files_scanned;
extern volatile sig_atomic_t g_sigusr1_count;
extern volatile sig_atomic_t g_worker_matches[8];
extern volatile sig_atomic_t g_worker_files[8];
extern volatile sig_atomic_t g_sigterm_sent[8];
extern volatile sig_atomic_t g_worker_done[8];
extern int g_match_pipes[8][2];

void parse_arguments(int argc, char **argv, t_args *args);
int regex(t_args *args, const char *filename);
void check_directory(const char *path);
int search_directory(const char *path, t_args *args, int *files_scanned);
int root_subdirs(const char *root);
void print_tree(const char *root, t_match *matches, int count);
void print_summary(void);
void setup_sigchld_handler(void);
void print_summary(void);
void setup_sigterm_handler(void);
void sigusr1_handler(int sig);
void notify_parent(void);
void set_parent_pid(pid_t parent_pid);
void setup_sigint_handler(void);
void sigint_handler(int sig);
void set_worker_index(int idx);
void record_match(void);
void listen_sigusr1(void);
void ignore_child_sigint(void);
void close_child_pipes(int num_workers, int i);
int record_match_and_file(int i, int num_subdirs, t_args *args, char subdirs[][4096]);
int fill_subdirs(const char *root, char subdirs[][4096], int max);
int check_num_subdirs(int num_subdirs, t_args *args);
int create_workers(t_args *args);
void parent_process(t_match *all_matches, int *total_match_count);
void sigterm_handler(int sig);
void sigint_handler(int sig);
void sigchld_handler(int sig);
int search_directory_collect(const char *path, t_args *args,
                              t_match *all_matches, int *total_match_count);

#endif