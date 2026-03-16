#include "../inc/functions.h"
#include <ctype.h>

int regex(t_args *args, const char *filename)
{
    char *pattern = args->pattern;

    if (*pattern == '\0')
        return *filename == '\0';

    char c = tolower((unsigned char)*pattern);

    if (*(pattern + 1) == '+')
    {
        int i = 0;

        if (filename[0] == '\0' || tolower((unsigned char)filename[0]) != c)
            return 0;

        while (filename[i] && tolower((unsigned char)filename[i]) == c)
        {
            t_args temp = *args;
            temp.pattern = pattern + 2;

            if (regex(&temp, filename + i + 1))
                return 1;

            i++;
        }
        return 0;
    }
    else
    {
        if (*filename && tolower((unsigned char)*filename) == c)
        {
            t_args temp = *args;
            temp.pattern = pattern + 1;
            return regex(&temp, filename + 1);
        }
        return 0;
    }
}