#include "../../inc/functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s -f <floors> -w <word_carriers> -l <letter_carriers>\n"
        "       %*s -s <sorting_procs> -c <max_words_floor>\n"
        "       %*s -d <delivery_cap> -r <reposition_cap>\n"
        "       %*s -i <input_file> -o <output_file>\n",
        prog,
        (int)strlen(prog) + 7, "",
        (int)strlen(prog) + 7, "",
        (int)strlen(prog) + 7, "");
}

void parse_args(int argc, char **argv, t_config *cfg)
{
    int opt;

    memset(cfg, 0, sizeof(t_config));

    while ((opt = getopt(argc, argv, "f:w:l:s:c:d:r:i:o:")) != -1){
        switch (opt){
            case 'f': cfg->num_floors                   = atoi(optarg); break;
            case 'w': cfg->word_carriers_per_floor      = atoi(optarg); break;
            case 'l': cfg->letter_carriers_per_floor    = atoi(optarg); break;
            case 's': cfg->sorting_processes_per_floor  = atoi(optarg); break;
            case 'c': cfg->max_words_per_floor          = atoi(optarg); break;
            case 'd': cfg->delivery_elevator_capacity   = atoi(optarg); break;
            case 'r': cfg->reposition_elevator_capacity = atoi(optarg); break;
            case 'i': cfg->input_file                   = optarg;       break;
            case 'o': cfg->output_file                  = optarg;       break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

void validate_args(t_config *cfg)
{
    int ok = 1;

    if (cfg->num_floors < 1)
        { fprintf(stderr, "Error: -f (num_floors) must be >= 1\n");              ok = 0; }
    if (cfg->word_carriers_per_floor < 1)
        { fprintf(stderr, "Error: -w (word_carriers) must be >= 1\n");           ok = 0; }
    if (cfg->letter_carriers_per_floor < 1)
        { fprintf(stderr, "Error: -l (letter_carriers) must be >= 1\n");         ok = 0; }
    if (cfg->sorting_processes_per_floor < 1)
        { fprintf(stderr, "Error: -s (sorting_processes) must be >= 1\n");       ok = 0; }
    if (cfg->max_words_per_floor < 1)
        { fprintf(stderr, "Error: -c (max_words_per_floor) must be >= 1\n");     ok = 0; }
    if (cfg->delivery_elevator_capacity < 1)
        { fprintf(stderr, "Error: -d (delivery_capacity) must be >= 1\n");       ok = 0; }
    if (cfg->reposition_elevator_capacity < 1)
        { fprintf(stderr, "Error: -r (reposition_capacity) must be >= 1\n");     ok = 0; }
    if (!cfg->input_file)
        { fprintf(stderr, "Error: -i (input_file) is required\n");               ok = 0; }
    if (!cfg->output_file)
        { fprintf(stderr, "Error: -o (output_file) is required\n");              ok = 0; }

    if (cfg->input_file)
    {
        int fd = open(cfg->input_file, O_RDONLY);
        if (fd < 0){
            fprintf(stderr, "Error: Cannot open input file '%s'\n", cfg->input_file);
            ok = 0;
        }
        else
            close(fd);
    }

    if (!ok){
        print_usage("./hw3");
        exit(EXIT_FAILURE);
    }
}