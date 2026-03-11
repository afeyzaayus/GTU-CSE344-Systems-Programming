#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "./includes/functions.h"
#include <sys/stat.h>
#include <limits.h>

void search_directory_internal(const char *curr_path, t_program *program, int depth, t_path_info *parent_info) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;

    if (!(dir = opendir(curr_path))) {
        write(STDERR_FILENO, "Directory couldn't be opened.\n", 30);
        return ;
    }

    char full_path[1024];

    while (1)
    {
        entry = readdir(dir); // açılmış kalöredeki dosyaları tek tek oku
        if (!entry) // klaörün sonu demek
            break;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        strcpy(full_path, curr_path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        if (lstat(full_path, &st) == -1){
            write(STDERR_FILENO, "lstat failed\n", 13);
            continue;
        }

        int matched = compare(&st, program, entry->d_name);
        
        if (matched) {
            program->found_count++;
            print_path(parent_info);

            // If it's a directory but it ALSO matched our criteria, it might be printed now.
            // We'll set a local printed flag to true so we don't print it twice if we go inside.
            print_tree(entry->d_name, depth + 1, S_ISDIR(st.st_mode));
        }

        if (S_ISDIR(st.st_mode)) { // içinde başka klasörler varsa diye
            t_path_info current_info;
            current_info.name = entry->d_name;
            current_info.depth = depth + 1;
            current_info.printed = matched ? 1 : 0;
            current_info.parent = parent_info;

            search_directory_internal(full_path, program, depth + 1, &current_info);
        }
    }

    closedir(dir);
}

void search_directory(const char *curr_path, t_program *program, int depth) {
    search_directory_internal(curr_path, program, depth, NULL);
}