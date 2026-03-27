#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "../inc/functions.h"

int root_subdirs(const char *root)
{
    DIR *dir = opendir(root);
    if (!dir) return -1;

    struct dirent *entry;
    struct stat st;
    char full_path[4096];
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
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

int fill_subdirs(const char *root, char subdirs[][4096], int max)
{
    DIR *dir = opendir(root);
    if (!dir) return -1;

    struct dirent *entry;
    struct stat st;
    char full_path[4096];
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", root, entry->d_name);
        if (lstat(full_path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            strncpy(subdirs[count], full_path, 4095);
            subdirs[count][4095] = '\0';
            count++;
        }
    }
    closedir(dir);
    return count;
}

int check_num_subdirs(int num_subdirs, t_args *args)
{
    /* num_subdirs == 0 durumu artık processes.c'de handle ediliyor */
    if (num_subdirs < args->num_workers) {
        printf("Notice: only %d subdirectories found; using %d workers instead of %d.\n",
               num_subdirs, num_subdirs, args->num_workers);
        args->num_workers = num_subdirs;
    }
    return 1;
}