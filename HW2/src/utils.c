#include "../inc/functions.h"
#include <unistd.h>

void close_child_pipes(int num_workers, int i){
    close(g_pipes[i][0]);
    close(g_match_pipes[i][0]);

    for (size_t k = 0; k < num_workers; k++) {
        if (k != i) {
            close(g_pipes[k][0]);
            close(g_pipes[k][1]);
            close(g_match_pipes[k][0]);
            close(g_match_pipes[k][1]);
        }
    }
}

int record_match_and_file(int i, int num_subdirs, t_args *args, char subdirs[][4096]){
    int match_count   = 0;
    int files_scanned = 0;

    for (size_t j = i; j < num_subdirs; j += args->num_workers)
        match_count += search_directory(subdirs[j], args, &files_scanned);

    /* Önce match pipe'ı kapat (parent 2. döngüde EOF alır) */
    close(g_match_pipes[i][1]);

    /* Sonra results yaz ve kapat (parent 1. döngüde bunu bekliyor) */
    int results[2] = {match_count, files_scanned};
    write(g_pipes[i][1], results, sizeof(results));
    close(g_pipes[i][1]);
    return match_count;
}