#ifndef BUFFER_H
# define BUFFER_H

# include "log_entry.h"

typedef struct s_circular_buffer{
    t_log_entry    *data;
    int             capacity;
    int             head;
    int             tail;
    int             count;
}   t_circular_buffer;

int     buffer_init(t_circular_buffer *buf, int capacity);
int     buffer_push(t_circular_buffer *buf, t_log_entry *entry);
int     buffer_pop(t_circular_buffer *buf, t_log_entry *entry);
void    buffer_free(t_circular_buffer *buf);

#endif