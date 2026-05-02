#include "../../inc/parser.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void init_args(t_args *arg) {
    memset(arg, 0, sizeof(t_args));
    arg->timeout = 10;
}

int parse_args(int argc, char **argv, t_args *arg) {
    int opt;

    while ((opt = getopt(argc, argv, "c:f:k:t:w:a:b:d:T:o:O:")) != -1) {
        switch (opt) {
            case 'c':
                arg->config_file = optarg;
                break;
            case 'f':
                arg->filter_file = optarg;
                break;
            case 'k':
                arg->keywords_raw = optarg;
                break;
            case 't':
                arg->reader_threads = atoi(optarg);
                break;
            case 'w':
                arg->worker_threads = atoi(optarg);
                break;
            case 'a':
                arg->cap_a = atoi(optarg);
                break;
            case 'b':
                arg->cap_b = atoi(optarg);
                break;
            case 'd':
                arg->cap_d = atoi(optarg);
                break;
            case 'T':
                arg->timeout = atoi(optarg);
                break;
            case 'o':
                arg->output_txt = optarg;
                break;
            case 'O':
                arg->output_bin = optarg;
                break;
            default:
                return (0);
        }
    }
    return (1);
}

int validate_args(t_args *arg) {
    if (!arg->config_file || !arg->filter_file || !arg->keywords_raw ||
            !arg->output_txt || !arg->output_bin )
        return (0);

    if (arg->reader_threads < 1) return (0);
    if (arg->worker_threads < 1 || arg->worker_threads > 64) 
        return (0);
    if (arg->cap_a < 4) return (0);
    if (arg->cap_b < 4) return (0);
    if (arg->cap_d < 2) return (0);
    if (arg->timeout < 1) return (0);
    if (access(arg->config_file, R_OK) != 0)
        return (0);
    if (access(arg->filter_file, R_OK) != 0)
        return (0);
    return (1);
}

int parse_keywords(t_args *arg) {
    char    *copy;
    char    *token;
    int     i = 0;

    copy = strdup(arg->keywords_raw);
    if (!copy) return (perror("Strdup error!"), 0);
    token = strtok(copy, ",");

    while (token){
        if (i >= MAX_KEYWORDS)
            return (perror("Error: maximum keyword limit exceeded"), free(copy), 0);

        if (*token == '\0') 
            return (perror("Error: empty keyword detected in -k argument."), free(copy), 0);

        arg->keywords[i] = strdup(token);
        if (!arg->keywords[i])
            return (perror("Error: strdup"), free(copy), 0);

        token = strtok(NULL, ",");
        i++;
    }

    arg->keyword_count = i;
    free(copy);
    if (i == 0) 
        return (perror("Error: no valid keywords provided in -k argument."), 0);
    return (1);
}

int start_parsing(int argc, char **argv, t_args *arg){
    init_args(arg);

    if (!parse_args(argc, argv, arg))
        return (0);

    if (!validate_args(arg)) {
        perror("ERROR: Missing argument or invalid number!");
        return (0);
    }

    if (!parse_keywords(arg))
        return (0);

    if (!parse_config_file(arg))
        return (0);

    if (!priority_parser(arg))
        return (0);

    return (1);
}

void free_args(t_args *arg) {
    for (int i = 0; i < arg->keyword_count; i++) {
        free(arg->keywords[i]);
        arg->keywords[i] = NULL;
    }
    for (int i = 0; i < arg->file_count; i++) {
        free(arg->log_files[i]);
        arg->log_files[i] = NULL;
    }
    for (int i = 0; i < arg->priority_count; i++) {
        free(arg->priority_sources[i]);
        arg->priority_sources[i] = NULL;
    }
}
