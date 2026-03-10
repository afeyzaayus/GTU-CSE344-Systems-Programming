#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
//#include <./includes/struct.h>
#include "./includes/functions.h"
#include <sys/stat.h>
#include <limits.h>

int regex_match_recursive(const char *pattern, const char *str) {
    if (*pattern == '\0') return *str == '\0';
    
    char c = tolower((unsigned char)*pattern);
    char next = *(pattern + 1);
    
    if (next == '+') {
        int i = 0;
        // The character 'c' must sequence at least 1 time
        if (tolower((unsigned char)str[i]) != c)
            return 0;

        while (str[i] != '\0' && tolower((unsigned char)str[i]) == c) {
            if (regex_match_recursive(pattern + 2, str + i + 1))
                return 1;
            i++;
        }
        return 0;
    } else {
        if (*str != '\0' && tolower((unsigned char)*str) == c)
            return regex_match_recursive(pattern + 1, str + 1);
        return 0;
    }
}

int regex_match(t_program *program, const char *filename){
    return regex_match_recursive(program->criteria.filename, filename);
}

