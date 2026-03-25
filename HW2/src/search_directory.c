#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "../inc/functions.h"

void check_directory(const char *path)
{
    DIR *dir = opendir(path);

    if (!dir)
    {
        perror("ERROR: invalid directory");
        exit(EXIT_FAILURE);
    }

    closedir(dir);
}

int search_directory(const char *path, t_args *args)
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int total = 0;

    dir = opendir(path);
    if (!dir)
        return;

    char full_path[1024];

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path),
                 "%s/%s", path, entry->d_name);

        if (lstat(full_path, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode))
        {
            total += search_directory(full_path, args);
        }
        else // if (S_ISREG(st.st_mode)) // 🔥 önemli fix
        {
            if (regex(args, entry->d_name))
            {
                if (!args->s_flag || st.st_size >= args->min_size)
                {
                    printf("[Worker PID:%d] MATCH: %s (%ld bytes)\n",
                        getpid(), full_path, st.st_size);
                    total++;
                }
            }
        }
    }

    closedir(dir);
    return total;
}