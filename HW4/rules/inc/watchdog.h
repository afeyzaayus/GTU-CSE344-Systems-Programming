#ifndef WATCHDOG_H
# define WATCHDOG_H

# include <signal.h>
# include <sys/types.h>
# include "argument_parsing.h"

// --------------------------------------------------
// Watchdog thread'e geçilen argüman struct'ı
// --------------------------------------------------
typedef struct s_watchdog_args
{
    int                 (*pipe_fds)[2];     // pipe_fds[reader_i][0=read,1=write]
    int                 reader_count;       // kaç reader var
    char                **log_files;        // dosya isimleri (log için)
    pid_t               *all_pids;          // tüm child pid'leri
    int                 pid_count;          // kaç child var
    volatile sig_atomic_t *shutdown;        // parent bu flag'i 1 yaparsa çık
}   t_watchdog_args;

// --------------------------------------------------
// FONKSIYON PROTOTIPI
// --------------------------------------------------
void    *watchdog_thread(void *arg);

#endif