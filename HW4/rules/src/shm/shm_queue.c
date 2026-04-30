#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "shm.h"

int region_a_push(t_region_a *a, t_log_entry *entry)
{
    if (a->count == a->capacity)
        return (0);
    a->data[a->tail] = *entry;
    a->tail = (a->tail + 1) % a->capacity;
    a->count++;
    return (1);
}
 
int region_a_pop(t_region_a *a, t_log_entry *entry)
{
    if (a->count == 0)
        return (0);
    *entry = a->data[a->head];
    a->head = (a->head + 1) % a->capacity;
    a->count--;
    return (1);
}
 
int region_b_push(t_region_b *b, t_log_entry *entry)
{
    if (b->count == b->capacity)
        return (0);
    b->data[b->tail] = *entry;
    b->tail = (b->tail + 1) % b->capacity;
    b->count++;
    return (1);
}
 
int region_b_pop(t_region_b *b, t_log_entry *entry)
{
    if (b->count == 0)
        return (0);
    *entry = b->data[b->head];
    b->head = (b->head + 1) % b->capacity;
    b->count--;
    return (1);
}
 
int region_d_push(t_region_d *d, t_log_entry *entry)
{
    if (d->count == d->capacity)
        return (0);
    d->data[d->tail] = *entry;
    d->tail = (d->tail + 1) % d->capacity;
    d->count++;
    return (1);
}
 
int region_d_pop(t_region_d *d, t_log_entry *entry)
{
    if (d->count == 0)
        return (0);
    *entry = d->data[d->head];
    d->head = (d->head + 1) % d->capacity;
    d->count--;
    return (1);
}