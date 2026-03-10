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

typedef enum e_error {
    ERR_NO_VALUE,
    ERR_INVALID_PARAM,
    ERR_BAD_FORMAT,
    ERR_NO_TARGET_DIR,
    ERR_NO_CRITERIA
} t_error;

typedef struct path_info {
    const char *name;
    int depth;
    int printed;
    struct path_info *parent;
} t_path_info;

#endif