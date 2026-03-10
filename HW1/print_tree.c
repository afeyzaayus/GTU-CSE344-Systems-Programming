#include "./includes/struct.h"
#include <unistd.h>
#include <string.h>

void print_tree(const char *name, int depth)
{
    int dash_count;

    write(1, "|", 1);

    dash_count = 2 + (depth - 1) * 4;

    for (int i = 0; i < dash_count; i++)
        write(1, "-", 1);

    write(1, name, strlen(name));
    write(1, "\n", 1);
}



void print_path(t_path_info *p) {
    if (!p || p->printed) return;
    print_path(p->parent);
    print_tree(p->name, p->depth);
    p->printed = 1;
}