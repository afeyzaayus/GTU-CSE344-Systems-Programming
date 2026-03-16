#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../inc/structs.h"

void parse_arguments(int argc, char **argv, t_args *args)
{
    int opt;

    while ((opt = getopt(argc, argv, "d:n:f:s:")) != -1)
    {
        switch (opt)
        {
            case 'd':
                args->root_dir = optarg;
                args->d_flag = 1;
                break;

            case 'n':
                args->num_workers = atoi(optarg);
                args->n_flag = 1;
                break;

            case 'f':
                args->pattern = optarg;
                args->f_flag = 1;
                break;

            case 's':
                args->min_size = atol(optarg);
                args->s_flag = 1;
                break;

            default:
                write(STDERR_FILENO, RED, sizeof(RED) - 1);
                write(STDERR_FILENO, "ERROR! : Invalid flag!\n", 23);
                write(STDERR_FILENO, RESET, sizeof(RESET) - 1);
                exit(EXIT_FAILURE);
        }
    }

    if (!args->d_flag || !args->n_flag || !args->f_flag)
    {
        write(STDERR_FILENO, RED, sizeof(RED) - 1);
        write(STDERR_FILENO, "ERROR! : Missing required argument!\n", 36);
        write(STDERR_FILENO, RESET, sizeof(RESET) - 1);
        exit(EXIT_FAILURE);
    }

    if (optind < argc)
    {
        write(STDERR_FILENO, RED, sizeof(RED) - 1);
        write(STDERR_FILENO, "ERROR! : Unexpected argument!\n", 30);
        write(STDERR_FILENO, RESET, sizeof(RESET) - 1);
        exit(EXIT_FAILURE);
    }

    if (args->num_workers < 2 || args->num_workers > 8)
    {
        write(STDERR_FILENO, RED, sizeof(RED) - 1);
        write(STDERR_FILENO, "ERROR! : num_workers must be between 2 and 8\n", 45);
        write(STDERR_FILENO, RESET, sizeof(RESET) - 1);
        exit(EXIT_FAILURE);
    }
}