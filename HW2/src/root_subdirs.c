#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "../inc/functions.h"


// bulduğu subdilreri subdirs arrayine yaz
int root_subdirs(const char *root)
{
    DIR *dir = opendir(root);
    if (!dir) return -1;

    struct dirent *entry;
    struct stat st;
    char full_path[4096];
    int count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", root, entry->d_name);

        if (lstat(full_path, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode))
            count++;
    }
    closedir(dir);
    return count;
}