//#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "./includes/functions.h"

void init_file_struct(t_file *file) {
    // int değerleri -1 ile başlatıyoruz (0 geçerli bir değer olabileceği için)
    file->link_count = -1;
    file->file_size = -1;
    
    // char pointer'ları NULL ile başlatıyoruz
    file->target_dir = NULL;
    file->filename = NULL;
    file->file_type = NULL;
    file->permissions = NULL;
}

int main(int argc, char **argv) {

    t_file criteria;
    init_file_struct(&criteria);
    if (!(parse_arguments(argc, argv, &criteria)))
        return 1;

    


    return 0;
}