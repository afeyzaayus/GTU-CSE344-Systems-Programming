#ifndef OUTPUT_H
#define OUTPUT_H

#include "structs.h"
#include "shm.h"

void setup_signal_handler(t_shm *shm, t_config *cfg,
                           pid_t *pid_list, int pid_count);
void write_output_file(t_shm *shm, t_config *cfg);
void print_summary(t_shm *shm);

#endif