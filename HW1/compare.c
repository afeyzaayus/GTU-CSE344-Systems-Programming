#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "./includes/functions.h"
#include <sys/stat.h>
#include <limits.h>

void mode_to_permission_string(struct stat st, char *perm){
    perm[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    perm[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    perm[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';

    perm[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    perm[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    perm[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';

    perm[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
    perm[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    perm[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
    perm[9] = '\0';
}

int is_correct_type(char type, struct stat *st){
    if (type == 'd' && !S_ISDIR(st->st_mode))
        return 0;
    if (type == 'f' && !S_ISREG(st->st_mode))
        return 0;
    if (type == 'l' && !S_ISLNK(st->st_mode))
        return 0;
    if (type == 'b' && !S_ISBLK(st->st_mode))
        return 0;
    if (type == 'c' && !S_ISCHR(st->st_mode))
        return 0;
    if (type == 'p' && !S_ISFIFO(st->st_mode))
        return 0;
    if (type == 's' && !S_ISSOCK(st->st_mode))
        return 0;
    return 1;
}

int compare(struct stat *st, t_program *program, const char *file_name){
    t_file criteria = program->criteria;
    if (criteria.filename != NULL){
        if (!regex_match(program, file_name))
            return 0;
    }

    if (criteria.file_size != -1){
        if (st->st_size != criteria.file_size)
            return 0;
    }

    if (criteria.link_count != -1){
        if ((int)st->st_nlink != criteria.link_count)
            return 0;
    }

    if (criteria.file_type != NULL){
        if (!is_correct_type(criteria.file_type[0], st))
            return 0;
    }

    if (criteria.permissions != NULL){
        char perm[10];
        mode_to_permission_string(*st, perm);
        if (strcmp(criteria.permissions, perm) != 0)
            return 0;
    }
    return 1;
}