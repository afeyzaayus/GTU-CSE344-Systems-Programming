#include <stdio.h>
#include <unistd.h>
#include "../inc/log_entry.h"
#include "reader.h"

int launch_system(t_args *args)
{
    printf("Launching system...\n");

    // 1. shared memory init
    // 2. pipes
    // 3. fork reader processes
    // 4. fork dispatcher
    // 5. fork analyzers
    // 6. fork aggregator
    // 7. watchdog thread

    return (0);
}

int main(int argc, char **argv)
{
    t_args args;

    if (!start_parsing(argc, argv, &args))
        return (1);

    run_reader(&args);

    return (0);
}