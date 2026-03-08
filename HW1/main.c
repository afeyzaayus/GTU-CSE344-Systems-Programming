//#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "./includes/functions.h"

void init_program(t_program *program) {
    program->criteria.link_count = -1;
    program->criteria.file_size = -1;
    program->criteria.target_dir = NULL;
    program->criteria.filename = NULL;
    program->criteria.file_type = NULL;
    program->criteria.permissions = NULL;
    program->found_count = 0;
}

int main(int argc, char **argv) {

    t_program program;
    init_program(&program);
    if (!(parse_arguments(argc, argv, &program.criteria)))
        return 1;

    if (!program.found_count){
        write(STDERR_FILENO, "No file found.\n", 15);
        return 1;
    }

    


    return 0;
}