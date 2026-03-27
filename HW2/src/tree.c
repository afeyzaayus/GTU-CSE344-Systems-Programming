#include "../inc/functions.h"
#include <stdio.h>
#include <string.h>

static void print_indent(int depth)
{
    printf("|");
    for (int i = 0; i < depth * 6; i++)
        printf("-");
}

static int split_path(const char *rel, char parts[][256])
{
    char tmp[4096];
    strncpy(tmp, rel, 4095);
    tmp[4095] = '\0';

    int   depth = 0;
    char *ptr   = tmp;
    char *token;

    while ((token = strsep(&ptr, "/")) != NULL && depth < 32) {
        if (*token == '\0') continue;
        strncpy(parts[depth], token, 255);
        parts[depth][255] = '\0';
        depth++;
    }
    return depth;
}

void print_tree(const char *root, t_match *matches, int count)
{
    if (count == 0) {
        printf("No matching files found.\n");
        return;
    }

    printf("%s\n", root);

    int root_len = strlen(root);

    char prev_parts[32][256];
    int  prev_depth = 0;
    memset(prev_parts, 0, sizeof(prev_parts));

    for (int i = 0; i < count; i++) {
        const char *rel = matches[i].path + root_len;

        char parts[32][256];
        int  depth = split_path(rel, parts);

        int common = 0;
        while (common < prev_depth - 1 && 
               common < depth  - 1 &&
               strcmp(prev_parts[common], parts[common]) == 0)
            common++;

        for (int d = common; d < depth; d++) {
            print_indent(d + 1);

            if (d == depth - 1)
                printf("-- %s (%ld bytes) [Worker %d]\n",
                       parts[d], matches[i].size, matches[i].worker_pid);
            else
                printf("-- %s\n", parts[d]);
        }

        prev_depth = depth;
        for (int d = 0; d < depth; d++)
            strncpy(prev_parts[d], parts[d], 255);
    }
}