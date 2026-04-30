// #ifndef ANALYZER_H
// # define ANALYZER_H

// # include "log_entry.h"
// # include "argument_parsing.h"

// typedef struct s_analysis_result
// {
//     double total_score;
//     double keyword_scores[8]; // MAX_KEYWORDS
// } t_analysis_result;

// int     get_level_weight(t_level level);
// int     count_keyword(char *text, char *word);

// void    analyze_entry(t_log_entry *e, t_args *arg);
// void    analyze_and_accumulate(t_log_entry *e, t_args *arg, t_analysis_result *res);

// void    init_analysis_result(t_analysis_result *res, int keyword_count);
// void    print_analysis_result(t_analysis_result *res, t_args *arg);

// #endif

#ifndef ANALYZER_H
# define ANALYZER_H

# include "log_entry.h"
# include "argument_parsing.h"
# include "shm.h"

// level_index: 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG
void    run_analyzer(t_args *args, t_shm *shm, int level_index);

int     get_level_weight(t_level level);
int     count_keyword(char *text, char *word);

#endif