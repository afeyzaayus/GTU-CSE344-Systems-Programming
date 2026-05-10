#include "../inc/signal_handler.h"

#include <signal.h>
#include <string.h>

volatile sig_atomic_t g_shutdown_requested = 0;

static void sigint_handler(int signum){
    (void)signum;
    g_shutdown_requested = 1;
}

int install_sigint_handler(void){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) return -1;
    return 0;
}