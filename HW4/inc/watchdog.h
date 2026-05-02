#ifndef WATCHDOG_H
# define WATCHDOG_H

# include <signal.h>
# include <sys/types.h>
# include "parser.h"

typedef struct s_watchdog_args{
    int                 (*pipe_fds)[2];     
    int                 reader_count;       
    char                **log_files;        
    pid_t               *all_pids;          
    int                 pid_count;          
    volatile sig_atomic_t *shutdown;        
}   t_watchdog_args;

void    *watchdog_thread(void *arg);

#endif