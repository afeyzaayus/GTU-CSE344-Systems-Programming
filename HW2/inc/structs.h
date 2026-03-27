#ifndef STRUCTS_H
#define STRUCTS_H

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"


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
