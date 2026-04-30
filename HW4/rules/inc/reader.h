#ifndef READER_H
# define READER_H

# include "log_entry.h"
# include "argument_parsing.h"
# include "shm.h"

// reader_index: kaçıncı reader olduğu (0, 1, 2, ...)
// pipe_write_fd: heartbeat mesajlarını buraya yazar
void    run_reader(t_args *args, t_shm *shm, int reader_index, int pipe_write_fd);

#endif