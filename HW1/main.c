#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include "./includes/functions.h"

volatile sig_atomic_t g_stop_requested = 0;

static void handle_sigint(int sig) {
    (void)sig;
    g_stop_requested = 1;
}

static int setup_signal_handlers(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        write(STDERR_FILENO, "Failed to register SIGINT handler.\n", 34);
        return 0;
    }
    return 1;
}

void init_program(t_program *program) {
    program->criteria.link_count = -1;
    program->criteria.file_size = -1;
    program->criteria.target_dir = NULL;
    program->criteria.filename = NULL;
    program->criteria.file_type = NULL;
    program->criteria.permissions = NULL;
    program->found_count = 0;
}

void check_signal(){
    if (g_stop_requested) {
        const char *msg = "Interrupted by CTRL-C. Resources released, exiting.\n";
        write(STDERR_FILENO, msg, 52);
        exit(130);
    }
}

void print_target_dir(const char *target_dir) {
    write(STDOUT_FILENO, "\033[31m", 5);
    write(STDOUT_FILENO, target_dir, strlen(target_dir));
    write(STDOUT_FILENO, "\033[0m\n", 5);
}

int main(int argc, char **argv) {
    t_program program;

    if (!setup_signal_handlers())
        return 1;

    init_program(&program);
    if (!(parse_arguments(argc, argv, &program.criteria)))
        return 1;

    check_signal();
    print_target_dir(program.criteria.target_dir);
    search_directory(program.criteria.target_dir, &program, 0);
    check_signal();

    if (!program.found_count) {
        write(STDERR_FILENO, "No file found.\n", 15);
        return 1;
    }
    return 0;
}