#include "../inc/functions.h"
#include "../inc/process_spawn.h"
#include "../inc/output.h"
#include "../inc/log.h"
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
    int          i;

    parse_args(argc, argv, &cfg);
    validate_args(&cfg);

    log_msg("Program is starting...\n");
    log_msg("Input file is being read...\n");

    words_input = read_input(cfg.input_file, &word_count);

    /* sorting_floor >= num_floors olan word varsa hata ver */
    validate_words(words_input, word_count, &cfg);

    log_msg("Shared memory is being initialized...\n");
    shm = shm_init(word_count, words_input, &cfg);

    for (i = 0; i < word_count; i++)
        free(words_input[i].word);
    free(words_input);

    log_msg("Synchronization primitives are created...\n");

    pid_list = malloc(sizeof(pid_t) * total_process_count(&cfg));
    if (!pid_list) { perror("ERROR: malloc pid_list"); exit(EXIT_FAILURE); }

    spawn_all(&shm, &cfg, pid_list, &pid_count);
    setup_signal_handler(&shm, &cfg, pid_list, pid_count);
    monitor_loop(&shm, &cfg, pid_list, pid_count);

    write_output_file(&shm, &cfg);
    print_summary(&shm);

    free(pid_list);
    shm_destroy(&shm, cfg.num_floors);
    return 0;
}