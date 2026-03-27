#include "../inc/functions.h"
#include <ctype.h>

static int match(const char *pattern, const char *filename)
{
    if (*pattern == '\0')
        return 1;

    char c = tolower((unsigned char)*pattern);

    if (*(pattern + 1) == '+') {
        int i = 0;

        if (filename[0] == '\0' || tolower((unsigned char)filename[0]) != c)
            return 0;

        while (filename[i] && tolower((unsigned char)filename[i]) == c) {
            if (match(pattern + 2, filename + i + 1))
                return 1;
            i++;
        }
        return 0;
    }
    else {
        if (*filename && tolower((unsigned char)*filename) == c)
            return match(pattern + 1, filename + 1);
        return 0;
    }
}

int regex(t_args *args, const char *filename)
{
    const char *pattern = args->pattern;

    for (; *filename != '\0'; filename++)
        if (match(pattern, filename))
            return 1;

    return match(pattern, filename);
}