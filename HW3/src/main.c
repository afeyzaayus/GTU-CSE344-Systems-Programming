#include "../inc/functions.h"
#include "../inc/process_spawn.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    t_config     cfg;
    t_line_input *words_input;
    int          word_count;
    t_shm        shm;
    pid_t        *pid_list;
    int          pid_count;

    parse_args(argc, argv, &cfg);
    validate_args(&cfg);

    printf("Program is starting...\n");
    printf("Input file is being read...\n");

    words_input = ft_read_input(cfg.input_file, &word_count);

    printf("Shared memory is being initialized...\n");
    shm = shm_init(word_count, words_input, &cfg);

    for (int i = 0; i < word_count; i++)
        free(words_input[i].word);
    free(words_input);

    printf("Synchronization primitives are created...\n");

    pid_list = malloc(sizeof(pid_t) * total_process_count(&cfg));
    if (!pid_list) {
        perror("ERROR: malloc pid_list");
        exit(EXIT_FAILURE);
    }

    spawn_all(&shm, &cfg, pid_list, &pid_count);

    monitor_loop(&shm, &cfg, pid_list, pid_count);

    /* TODO: output dosyası yaz, özet yazdır */

    free(pid_list);
    shm_destroy(&shm, cfg.num_floors);
    return 0;
}