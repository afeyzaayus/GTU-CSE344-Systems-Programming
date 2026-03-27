#include "../inc/functions.h"
#include <stdio.h>
#include <string.h>

static void print_indent(int depth)
{
    printf("|");
    for (int i = 0; i < depth * 6; i++)
        printf("-");
}

void print_tree(const char *root, t_match *matches, int count)
{
    if (count == 0) {
        printf("No matching files found.\n");
        return;
    }

    printf("%s\n", root);

    int root_len = strlen(root);

    for (int i = 0; i < count; i++) {
        const char *rel = matches[i].path + root_len;

        char tmp[4096];
        strncpy(tmp, rel, 4095);
        tmp[4095] = '\0';

        char  parts[32][256];
        int   depth = 0;
        char *ptr   = tmp;
        char *token;

        while ((token = strsep(&ptr, "/")) != NULL) {
            if (*token == '\0') continue;
            strncpy(parts[depth], token, 255);
            parts[depth][255] = '\0';
            depth++;
        }

        for (int d = 0; d < depth; d++) {
            print_indent(d + 1);

            if (d == depth - 1)
                printf("-- %s (%ld bytes) [Worker %d]\n",
                       parts[d], matches[i].size, matches[i].worker_pid);
            else
                printf("-- %s\n", parts[d]);
        }
    }
}