#include "../inc/functions.h"
#include <string.h>

int main(int argc, char **argv)
{
    t_args args;
    memset(&args, 0, sizeof(t_args));
    parse_arguments(argc, argv, &args);
    check_directory(args.root_dir);

    if (create_workers(&args) == -1)
        return 1;

    return 0;
}