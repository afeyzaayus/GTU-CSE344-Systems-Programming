#ifndef AGGREGATOR_H
# define AGGREGATOR_H

# include "parser.h"
# include "shm.h"

#define BINARY_MAGIC    0xC5E3440B
#define BINARY_VERSION  1

typedef struct s_bin_header{
    unsigned int    magic;
    unsigned int    version;
    unsigned int    num_levels;
    unsigned int    num_keywords;
    double          total_weighted;
    double          high_priority_weighted;
}   t_bin_header;

void    run_aggregator(t_args *args, t_shm *shm);

#endif