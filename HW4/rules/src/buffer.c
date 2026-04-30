#include <stdlib.h>
#include <string.h>
#include "buffer.h"

// --------------------------------------------------
// INIT
// --------------------------------------------------
int buffer_init(t_circular_buffer *buf, int capacity)
{
    buf->data = malloc(sizeof(t_log_entry) * capacity);
    if (!buf->data)
        return (0);

    buf->capacity = capacity;
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;

    return (1);
}

// --------------------------------------------------
// PUSH (Producer)
// --------------------------------------------------
int buffer_push(t_circular_buffer *buf, t_log_entry *entry)
{
    if (buf->count == buf->capacity)
        return (0); // buffer full

    buf->data[buf->tail] = *entry;

    buf->tail = (buf->tail + 1) % buf->capacity;
    buf->count++;

    return (1);
}

// --------------------------------------------------
// POP (Consumer)
// --------------------------------------------------
int buffer_pop(t_circular_buffer *buf, t_log_entry *entry)
{
    if (buf->count == 0)
        return (0); // buffer empty

    *entry = buf->data[buf->head];

    buf->head = (buf->head + 1) % buf->capacity;
    buf->count--;

    return (1);
}

// --------------------------------------------------
// FREE
// --------------------------------------------------
void buffer_free(t_circular_buffer *buf)
{
    if (buf->data)
        free(buf->data);
}