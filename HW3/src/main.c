#include "../inc/functions.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    t_config     cfg;
    t_line_input *words_input;
    int          word_count;
    t_shm        shm;
    int          i;

    parse_args(argc, argv, &cfg);
    validate_args(&cfg);

    printf("Program is starting...\n");
    printf("Input file is being read...\n");

    words_input = ft_read_input(cfg.input_file, &word_count);

    printf("Read %d words:\n", word_count);
    for (i = 0; i < word_count; i++)
        printf("  [%d] id=%-4d word=%-12s sorting_floor=%d\n",
               i, words_input[i].word_id,
               words_input[i].word,
               words_input[i].sorting_floor);

    printf("Shared memory is being initialized...\n");
    shm = shm_init(word_count, words_input, &cfg);

    /* words_input artık shm'e kopyalandı, serbest bırak */
    for (i = 0; i < word_count; i++)
        free(words_input[i].word);
    free(words_input);

    /* TODO: process spawning */

    shm_destroy(&shm, cfg.num_floors);
    return 0;
}