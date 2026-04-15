#ifndef LOG_H
#define LOG_H

#include <unistd.h>

void log_msg(const char *msg);
void log_fmt(const char *fmt, ...);
void log_err(const char *fmt, ...);

#endif