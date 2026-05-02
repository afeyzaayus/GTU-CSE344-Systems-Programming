#include <stdlib.h>
#include <string.h>
#include "../../inc/buffer.h"

int buffer_init(t_circular_buffer *buf, int capacity){
    buf->data = malloc(sizeof(t_log_entry) * capacity);
    if (!buf->data)
        return (0);

    buf->capacity = capacity;
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    return (1);
}

int buffer_push(t_circular_buffer *buf, t_log_entry *entry){
    if (buf->count == buf->capacity)
        return (0); 

    buf->data[buf->tail] = *entry;
    buf->tail = (buf->tail + 1) % buf->capacity;
    buf->count++;
    return (1);
}

int buffer_pop(t_circular_buffer *buf, t_log_entry *entry){
    if (buf->count == 0)
        return (0);

    *entry = buf->data[buf->head];
    buf->head = (buf->head + 1) % buf->capacity;
    buf->count--;
    return (1);
}

void buffer_free(t_circular_buffer *buf){
    if (buf->data)
        free(buf->data);
}