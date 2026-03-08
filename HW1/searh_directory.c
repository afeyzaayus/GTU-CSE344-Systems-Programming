#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <./includes/struct.h>
#include <sys/stat.h>
#include <limits.h>

// Verilen dizini aç 
// → içindeki her entry'yi incele 
// → kriterleri kontrol et 
// → gerekiyorsa yazdır 
// → eğer dizinse recursive devam et.


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


// ilerde düzelecek
int compare(struct stat *st, t_file *criteria, const char *file_name){
    if (criteria->filename != NULL)
    {
        if (strcmp(criteria->filename, file_name) != 0)
            return 0;
    }

    if (criteria->file_size != -1)
    {
        if (st->st_size != criteria->file_size)
            return 0;
    }

    if (criteria->link_count != -1)
    {
        if (st->st_nlink != criteria->link_count)
            return 0;
    }

    if (criteria->file_type != NULL)
    {
        char type = criteria->file_type[0];

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
    }

    if (criteria->permissions != NULL)
    {
        char perm[10];

        mode_to_permission_string(*st, perm);TRL-C handling

        if (strcmp(criteria->permissions, perm) != 0)
            return 0;
        
    }
    
    return 1;
}

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


void search_directory(const char *curr_path, t_program *program, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;

    if (!(dir = opendir(curr_path)))
    {
        write(STDERR_FILENO, "Directory couldn't be opened.\n", 30);
        return ;
    }

    char full_path[1024]; // realloc?

    while ((entry == readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        strcpy(full_path, curr_path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        if (lstat(full_path, &st) == -1){
            write(STDERR_FILENO, "lstat failed\n", 13);
            continue;
        }

        if (compare(&st, &program->criteria, entry->d_name)) {
            program->found_count++;
            print_tree(entry->d_name, depth + 1);
        }

        if (S_ISDIR(st.st_mode)) // içinde başka klasörler varsa diye
            search_directory(full_path, program, depth + 1);

    }

    closedir(dir);
    
}