#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct s_args {
    char *root_dir;
    int num_workers; 
    char *pattern; 
    int min_size; 

    int d_flag;
    int n_flag;
    int f_flag;
    int s_flag;
} t_args ;

typedef struct {
    char path[4096];
    int  worker_pid;
    long size;
} t_match;

#endif 
