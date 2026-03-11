#include "./includes/struct.h"
#include <unistd.h>
#include <string.h>

void print_tree(const char *name, int depth, int is_dir)
{
    int dash_count;

    write(1, "\033[1;33m", 7); // Sarı renk (branch)
    write(1, "|", 1);
    write(1, "\033[0m", 5); // Rengi sıfırla

    dash_count = 2 + (depth - 1) * 4;

    for (int i = 0; i < dash_count; i++)
        write(1, "-", 1);

    write(1, "\033[0m", 5); // Rengi sıfırla

    if (is_dir)
        write(1, "\033[0;32m", 7); // Yeşil renk (directory)
    else
        write(1, "\033[37m", 5); // Beyaz renk (dosya)

    write(1, name, strlen(name));
    write(1, "\033[0m\n", 5); // Rengi sıfırla ve yeni satır
}



void print_path(t_path_info *p) {
    if (!p || p->printed) return;
    print_path(p->parent);
    print_tree(p->name, p->depth, 1);
    p->printed = 1;
}