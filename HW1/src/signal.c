#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include "../includes/functions.h"

void handle_sigint(int sig) {
    (void)sig;
    g_stop_requested = 1;
}

int setup_signal_handlers(void) {
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

void check_signal(void){
    if (g_stop_requested) {
        const char *msg = "Interrupted by CTRL-C. Resources released, exiting.\n";
        write(STDERR_FILENO, "\033[34m", 5);
        write(STDERR_FILENO, msg, 52);
        write(STDERR_FILENO, "\033[0m", 4);
        exit(130);
    }
}
