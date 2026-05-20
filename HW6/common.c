#define _POSIX_C_SOURCE 200809L

#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

ssize_t write_all(int fd, const char *buf, size_t len){
    size_t total = 0;
    while (total < len) {
        ssize_t n = write(fd, buf + total, len - total);
        if (n < 0) {
            if (errno == EINTR) continue; 
            return -1;
        }
        if (n == 0) return -1;  
        total += (size_t)n;
    }
    return (ssize_t)total;
}

int send_line(int fd, const char *line){
    size_t len = strlen(line);
    int needs_newline = (len == 0 || line[len - 1] != '\n');

    if (write_all(fd, line, len) < 0) return -1;
    if (needs_newline) {
        if (write_all(fd, "\n", 1) < 0) return -1;
    }
    return 0;
}

void log_event(FILE *fp, const char *fmt, ...){
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);

    char ts[32];
    strftime(ts, sizeof(ts), "[%Y-%m-%d %H:%M:%S]", &tm_buf);

    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    fprintf(stdout, "%s [SERVER] %s\n", ts, msg);
    fflush(stdout);

    if (fp != NULL) {
        fprintf(fp, "%s [SERVER] %s\n", ts, msg);
        fflush(fp);
    }
}

void trim_newline(char *s){
    if (s == NULL) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}