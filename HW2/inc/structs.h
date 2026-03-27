#ifndef STRUCTS_H
#define STRUCTS_H

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"

// Yazı Renkleri (Kalın/Bold)
#define BOLD_RED     "\x1b[1;31m"
#define BOLD_GREEN   "\x1b[1;32m"
#define BOLD_YELLOW  "\x1b[1;33m"
#define BOLD_BLUE    "\x1b[1;34m"

// Arka Plan Renkleri
#define BG_RED     "\x1b[41m"
#define BG_GREEN   "\x1b[42m"
#define BG_BLUE    "\x1b[44m"

// Renk Sıfırlama (Her zaman sona eklenmeli!)
#define RESET   "\x1b[0m"

typedef struct s_args {
    char *root_dir; // -d
    int num_workers; // -r
    char *pattern; // -f
    int min_size; // optional -s

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
