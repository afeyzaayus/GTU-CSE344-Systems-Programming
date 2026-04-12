#include "../inc/log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LOG_BUF 512

void log_msg(const char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void log_fmt(const char *fmt, ...)
{
    char    buf[LOG_BUF];
    va_list ap;
    int     len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len > 0)
        write(STDOUT_FILENO, buf, (size_t)len);
}

void log_err(const char *fmt, ...)
{
    char    buf[LOG_BUF];
    va_list ap;
    int     len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len > 0)
        write(STDERR_FILENO, buf, (size_t)len);
}