#ifndef LOG_ENTRY_H
# define LOG_ENTRY_H

# define MAX_SOURCE_LEN 64
# define MAX_MESSAGE_LEN 512
# define TMP_BUF_SIZE 128

typedef enum e_level
{
    LV_ERROR = 0,
    LV_WARN,
    LV_INFO,
    LV_DEBUG,
    LV_INVALID
}   t_level;

typedef struct s_log_entry
{
    char    timestamp[20];
    t_level level;
    char    source[MAX_SOURCE_LEN];
    char    message[MAX_MESSAGE_LEN];
    int     is_eof;
}   t_log_entry;

int parse_log_line(char *line, t_log_entry *e);

#endif