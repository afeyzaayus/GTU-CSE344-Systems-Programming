#include "../../inc/argument_parsing.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int parse_config_file(t_args *arg) {
    FILE    *fp;
    char    line[1024];
    int     i;

    i = 0;
    fp = fopen(arg->config_file, "r");
    if (!fp)
        return (perror("Error: fopen"), 0);

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0')
            continue;

        if (i >= MAX_FILES) {
            perror("Error: too many log files in config");
            return (fclose(fp), 0);
        }

        arg->log_files[i] = strdup(line);
        if (!arg->log_files[i]) {
            perror("Error: strdup");
            return (fclose(fp), 0);
        }
        i++;
    }

    fclose(fp);
    if (i == 0) {
        perror("Error: config file contains no log files.");
        return (0);
    }
    arg->file_count = i;
    return (1);
}