#include "../inc/log.h"

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

static pthread_mutex_t log_mutex;
static int             log_initialized = 0;

void log_init(void){
    if (log_initialized) return;
    pthread_mutex_init(&log_mutex, NULL);
    log_initialized = 1;
}

void log_destroy(void){
    if (!log_initialized) return;
    pthread_mutex_destroy(&log_mutex);
    log_initialized = 0;
}

void log_printf(const char *fmt, ...){
    if (log_initialized) pthread_mutex_lock(&log_mutex);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fputc('\n', stdout);

    fflush(stdout);

    if (log_initialized) pthread_mutex_unlock(&log_mutex);
}