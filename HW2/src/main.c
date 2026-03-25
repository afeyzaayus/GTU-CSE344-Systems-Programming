#include "../inc/functions.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int create_workers(t_args *args, char subdirs[][1024]) {
    int num_subdirs = root_subdirs(args->root_dir, subdirs);

    if (num_subdirs == 0)
    {
        printf("Notice: no subdirectories found; parent will search root directly.\n");
        search_directory(args->root_dir, args);
        return 0;
    }

    if (num_subdirs < args->num_workers)
    {
        printf("Notice: only %d subdirectories found; using %d workers instead of %d.\n",
               num_subdirs, num_subdirs, args->num_workers);
        args->num_workers = num_subdirs;
    }

    pid_t pids[8]; // max 8 worker

    for (size_t i = 0; i < args->num_workers; i++)
    {
        pids[i] = fork();

        if (pids[i] == -1)
        {
            perror("Fork failed");
            return -1;
        }

        if (pids[i] == 0) // CHILD
        {
            int match_count = 0;

            for (size_t j = i; j < num_subdirs; j += args->num_workers)
                match_count += search_directory(subdirs[j], args);

            exit(match_count % 256);
        }
    }

    // PARENT: tüm child'ları bekle
    for (size_t i = 0; i < args->num_workers; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);

        if (WIFEXITED(status))
        {
            int match_count = WEXITSTATUS(status);
            printf("[Parent] Worker PID:%d finished with %d matches.\n",
                   pids[i], match_count);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    t_args args;
    char subdirs[100][1024];

    memset(&args, 0, sizeof(t_args));
    parse_arguments(argc, argv, &args);

    check_directory(args.root_dir);

    if (create_workers(&args, subdirs) == -1)
        return 1;
    
    return 0;
}