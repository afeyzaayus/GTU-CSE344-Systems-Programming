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


#endif