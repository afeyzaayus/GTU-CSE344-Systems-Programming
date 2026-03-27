#include "../inc/functions.h"
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

extern volatile sig_atomic_t g_partial_matches;
extern volatile sig_atomic_t g_files_scanned;
extern int g_match_pipes[8][2];
extern int g_worker_index;

void check_directory(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) {
        perror("ERROR: invalid directory");
        exit(EXIT_FAILURE);
    }
    closedir(dir);
}

int search_directory(const char *path, t_args *args, int *files_scanned)
{
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int total = 0;

    dir = opendir(path);
    if (!dir) return 0;

    char full_path[4096];

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (lstat(full_path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode))
        {
            total += search_directory(full_path, args, files_scanned);
        }
        else
        {
            (*files_scanned)++;
            g_files_scanned++;

            if (regex(args, entry->d_name))
            {
                if (!args->s_flag || st.st_size >= args->min_size)
                {
                    printf("[Worker PID:%d] MATCH: %s (%ld bytes)\n",
                           getpid(), full_path, st.st_size);

                    // Match'i pipe'a yaz
                    t_match m;
                    strncpy(m.path, full_path, 4095);
                    m.path[4095] = '\0';
                    m.worker_pid = getpid();
                    m.size       = st.st_size;
                    write(g_match_pipes[g_worker_index][1], &m, sizeof(t_match));

                    total++;
                    g_partial_matches++;
                    record_match();
                }
            }
        }
    }

    closedir(dir);
    return total;
}