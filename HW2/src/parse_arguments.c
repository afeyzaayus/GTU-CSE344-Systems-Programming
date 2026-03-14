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
                break;

            case 'n':
                args->num_workers = atoi(optarg);
                break;

            case 'f':
                args->pattern = optarg;
                break;

            case 's':
                args->min_size = atol(optarg);
                args->size_flag = 1;
                break;

            default:
                write(STDERR_FILENO, RED, sizeof(RED) - 1);
                write(STDERR_FILENO, "ERROR! : Invalid flag!\n", 30);
                write(STDERR_FILENO, RESET, sizeof(RESET) - 1);
                exit(EXIT_FAILURE);
        }
    }
}