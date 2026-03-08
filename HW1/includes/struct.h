#ifndef STRUCT_h
# define STRUCT_H

typedef struct s_file {
    int link_count;
    int file_size;
    char *target_dir;
    char *filename;
    char *file_type;
    char *permissions;
} t_file;

typedef struct s_program {
    t_file criteria;
    int found_count;
} t_program;


#endif