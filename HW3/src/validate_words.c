#include "../inc/functions.h"
#include "../inc/log.h"
#include <stdlib.h>

void validate_words(t_line_input *words, int word_count, t_config *cfg) {
    for (int i = 0; i < word_count; i++) {
        if (words[i].sorting_floor < 0 || words[i].sorting_floor >= cfg->num_floors) {
            log_err("Error: word '%s' (id=%d) has sorting_floor=%d"
                    " but num_floors=%d (valid: 0..%d)\n",
                    words[i].word,
                    words[i].word_id,
                    words[i].sorting_floor,
                    cfg->num_floors,
                    cfg->num_floors - 1);
            exit(EXIT_FAILURE);
        }
    }
}