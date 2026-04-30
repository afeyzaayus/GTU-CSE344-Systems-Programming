#ifndef ARGUMENT_PARSING_H
# define ARGUMENT_PARSING_H

# define MAX_KEYWORDS 8
# define MAX_FILES 128
# define MAX_SOURCES 128

typedef struct s_args {
    char    *config_file;
    char    *filter_file;
    char    *output_txt;
    char    *output_bin;
    char    *keywords_raw;
    char    *keywords[MAX_KEYWORDS];
    char    *log_files[MAX_FILES];
    char    *priority_sources[MAX_SOURCES];
    int     priority_count;
    int     file_count;
    int     keyword_count;
    int     reader_threads;
    int     worker_threads;
    int     cap_a;
    int     cap_b;
    int     cap_d;
    int     timeout;
    
}   t_args;

int start_parsing(int argc, char **argv, t_args *arg);
int parse_config_file(t_args *arg);
int priority_parser(t_args *arg);


#endif