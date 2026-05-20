#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#define MAX_LINE_LEN       512  
#define MAX_USERNAME_LEN   32   
#define MAX_INGREDIENT_LEN 17  
#define MAX_TYPE_LEN       16 

ssize_t write_all(int fd, const char *buf, size_t len);

int send_line(int fd, const char *line);

void log_event(FILE *fp, const char *fmt, ...);

void trim_newline(char *s);

#endif 