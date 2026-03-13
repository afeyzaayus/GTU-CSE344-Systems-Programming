#include <ctype.h>
#include "../includes/functions.h"

int regex_match(t_file *criteria, const char *filename){
    char *pattern = criteria->filename;
    
    if (*pattern == '\0') return *filename == '\0';
    
    char c = tolower((unsigned char)*pattern);
    char next = *(pattern + 1);
    
    if (next == '+') {
        int i = 0;
        
        if (tolower((unsigned char)filename[i]) != c)
            return 0;

        while (filename[i] != '\0' && tolower((unsigned char)filename[i]) == c) {
            t_file temp_criteria = *criteria;
            temp_criteria.filename = pattern + 2;
            if (regex_match(&temp_criteria, filename + i + 1))
                return 1;
            i++;
        }
        return 0;
    } else {
        if (*filename != '\0' && tolower((unsigned char)*filename) == c) {
            t_file temp_criteria = *criteria;
            temp_criteria.filename = pattern + 1;
            return regex_match(&temp_criteria, filename + 1);
        }
        return 0;
    }
}

